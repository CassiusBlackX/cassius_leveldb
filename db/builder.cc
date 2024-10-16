// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/builder.h"

#include "db/dbformat.h"
#include "db/filename.h"
#include "db/table_cache.h"
#include "db/version_edit.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/iterator.h"

#ifdef ZAL_TIMER
#include "zal_utils.h"
#endif
#ifdef LOG_SST
#include "zal_utils.h"
extern zal_utils::ThreadSafeQueue<zal_utils::table_info> build_table_queue;
#endif

namespace leveldb {

Status BuildTable(const std::string& dbname, Env* env, const Options& options,
                  TableCache* table_cache, Iterator* iter, FileMetaData* meta) {
  #ifdef ZAL_TIMER
  zal_utils::FunctionTimer* BuildTable_timer = new zal_utils::FunctionTimer("BuildTable");
  #endif
  Status s;
  meta->file_size = 0;
  iter->SeekToFirst();

  std::string fname = TableFileName(dbname, meta->number);
  if (iter->Valid()) {
    WritableFile* file;
    s = env->NewWritableFile(fname, &file);
    if (!s.ok()) {
      return s;
    }

    #ifdef ZAL_TIMER
    zal_utils::FunctionTimer* TableBuilder_timer = new zal_utils::FunctionTimer(BuildTable_timer, "TableBuilder");
    #endif
    TableBuilder* builder = new TableBuilder(options, file);
    meta->smallest.DecodeFrom(iter->key());
    Slice key;
    for (; iter->Valid(); iter->Next()) {
      key = iter->key();
      builder->Add(key, iter->value());
    }
    if (!key.empty()) {
      meta->largest.DecodeFrom(key);
    }

    // Finish and check for builder errors
    s = builder->Finish();
    if (s.ok()) {
      meta->file_size = builder->FileSize();
      assert(meta->file_size > 0);
    }
    delete builder;
    #ifdef ZAL_TIMER
    delete TableBuilder_timer;
    zal_utils::FunctionTimer* BuildTable_FileCheck_timer = new zal_utils::FunctionTimer(BuildTable_timer, "FileCheck");
    #endif

    // Finish and check for file errors
    if (s.ok()) {
      s = file->Sync();
    }
    if (s.ok()) {
      s = file->Close();
    }
    delete file;
    file = nullptr;

    if (s.ok()) {
      // Verify that the table is usable
      Iterator* it = table_cache->NewIterator(ReadOptions(), meta->number,
                                              meta->file_size);
      s = it->status();
      delete it;
    }
    #ifdef ZAL_TIMER
    delete BuildTable_FileCheck_timer;
    #endif
  }

  // Check for input iterator errors
  if (!iter->status().ok()) {
    s = iter->status();
  }

  if (s.ok() && meta->file_size > 0) {
    // Keep it
    #ifdef LOG_SST
    build_table_queue.push(zal_utils::table_info(meta->number, meta->smallest.user_key().ToString(), meta->largest.user_key().ToString(), meta->file_size));
    #endif
  } else {
    env->RemoveFile(fname);
  }


  #ifdef ZAL_TIMER
  delete BuildTable_timer;
  #endif
  return s;
}

}  // namespace leveldb
