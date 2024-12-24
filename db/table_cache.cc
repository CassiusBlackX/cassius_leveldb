// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/table_cache.h"

#include "db/dbformat.h"
#include "db/filename.h"
#include "leveldb/env.h"
#include "leveldb/table.h"
#include "util/coding.h"

namespace leveldb {

struct TableAndFile {
  RandomAccessFile** file;
  Table* table;
};

static void DeleteEntry(const Slice& key, void* value) {
  TableAndFile* tf = reinterpret_cast<TableAndFile*>(value);
  delete tf->table;
  delete tf->file[0];
  free(tf->file);
  delete tf;
}

static void UnrefEntry(void* arg1, void* arg2) {
  Cache* cache = reinterpret_cast<Cache*>(arg1);
  Cache::Handle* h = reinterpret_cast<Cache::Handle*>(arg2);
  cache->Release(h);
}

TableCache::TableCache(const std::string& dbname, const Options& options,
                       int entries, Ecpath& ecpath)
    : env_(options.env),
      dbname_(dbname),
      options_(options),
      cache_(NewLRUCache(entries)),
      ecpath_(ecpath) {}

TableCache::~TableCache() { delete cache_; }

Status TableCache::FindTable(uint64_t file_number, uint64_t file_size,
                             Cache::Handle** handle, int level) {
  Status s;
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  Slice key(buf, sizeof(buf));
  *handle = cache_->Lookup(key);

  // added by lzy .
  int ec_m = config::ec_m;
  int ec_k = config::ec_k;
  int ec_p = config::ec_p;

  if (*handle == nullptr) {
    std::string fname[ec_m];
    RandomAccessFile** file = (RandomAccessFile **)malloc(sizeof(RandomAccessFile *) * ec_m);
    Table* table = nullptr;
    if(level<=config::maxlowlevel)
    {
      fname[0] = TableFileName(dbname_, file_number);
      s = env_->NewRandomAccessFile(fname[0], &file[0]);
    }
    else
      for(int i=0;i<ec_m;i++)
      {
        fname[i] = ParityBlockFileName(ecpath_.getEcpath()[i], file_number, i);
        s = env_->NewRandomAccessFile(fname[i], &file[i]); 
      }
    s = Table::Open(options_, file, file_size, &table, level);

    if (!s.ok()) {
      assert(table == nullptr);
      if(level<=config::maxlowlevel)
        delete file[0];
      else
        for(int i=0;i<ec_m;i++)
          delete file[i];
      free(file);
      // We do not cache error results so that if the error is transient,
      // or somebody repairs the file, we recover automatically.
    } else {
      TableAndFile* tf = new TableAndFile;
      tf->file = file;
      tf->table = table;
      *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
    }
  }
  return s;
}

Iterator* TableCache::NewIterator(const ReadOptions& options,
                                  uint64_t file_number, uint64_t file_size, int level,
                                  Table** tableptr) {
  if (tableptr != nullptr) {
    *tableptr = nullptr;
  }

  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle, level);
  if (!s.ok()) {
    return NewErrorIterator(s);
  }

  Table* table = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
  leveldb::ReadOptions options_new = options;
  options_new.level = level;
  //printf("number:%d options_new.level:%d\n",file_number,options_new.level);
  Iterator* result = table->NewIterator(options_new);
  result->RegisterCleanup(&UnrefEntry, cache_, handle);
  if (tableptr != nullptr) {
    *tableptr = table;
  }
  return result;
}

Status TableCache::Get(const ReadOptions& options, uint64_t file_number,
                       uint64_t file_size, const Slice& k, void* arg,
                       void (*handle_result)(void*, const Slice&,
                                             const Slice&), int level) {
  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle, level);
  if (s.ok()) {
    Table* t = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
    ReadOptions options_new = options;
    options_new.level = level;
    s = t->InternalGet(options_new, k, arg, handle_result);
    cache_->Release(handle);
  }
  return s;
}

void TableCache::Evict(uint64_t file_number) {
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  cache_->Erase(Slice(buf, sizeof(buf)));
}

// added by lzy .
void TableCache::RafileChanger(uint64_t file_number, RandomAccessFile** file, int old_level)
{
  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, 0, &handle, old_level);
  assert(s.ok());
  
  Table* t = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
  RandomAccessFile** f = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->file;

  t->filechanger(file, old_level+1);
  f = file;
}

}  // namespace leveldb
