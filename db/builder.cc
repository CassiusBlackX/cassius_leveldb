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
#include "iostream"

#include "include/leveldb/timer.h"

#ifdef STRIPE_RECORDER
#include "zal_utils.h"
extern zal_utils::ThreadSafeSet<zal_utils::table_info> table_info_set;
#endif

namespace leveldb {

Status BuildTable(const std::string& dbname, Env* env, const Options& options,
                  TableCache* table_cache, Iterator* iter, FileMetaData* meta, Ecpath& ecpath) {
  long long BT_start_time = getCurrentTime();
  Status s;
  meta->file_size = 0;
  iter->SeekToFirst();

  // added by lzy .
  int level = 0; 
  int ec_m = config::ec_m;
  int ec_k = config::ec_k;
  int ec_p = config::ec_p;

  std::string fname[ec_m];
  if (iter->Valid()) {
    WritableFile** file = (WritableFile **)malloc(sizeof(WritableFile *) * ec_m);

    if(level<=config::maxlowlevel)
    {
      fname[0] = TableFileName(dbname, meta->number);
      s = env->NewWritableFile(fname[0], &file[0]);
    }
    else
      for(int i=0;i<ec_m;i++)
      {
        fname[i] = ParityBlockFileName(ecpath.getEcpath()[i], meta->number, i);
        s = env->NewWritableFile(fname[i], &file[i]); 
      }
    if (!s.ok()) {
      return s;
    }
    if(options.level)
      printf("BuildTable error at options level : %d\n",options.level);
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
    long long Finish_start_time = getCurrentTime();
    s = builder->Finish();
    long long Finish_end_time = getCurrentTime();
    if (s.ok()) {
      meta->file_size = builder->FileSize();
      assert(meta->file_size > 0);
    }
    delete builder;
    // Finish and check for file errors
    if (s.ok()) {
      if(level<=config::maxlowlevel)
        s = file[0]->Sync();
      else
        for(int i=0;i<ec_m;i++)
          s = file[i]->Sync();
    }
    if (s.ok()) {
      if(level<=config::maxlowlevel)
        s = file[0]->Sync();
      else
        for(int i=0;i<ec_m;i++)
          s = file[i]->Close();
    }
    if(level<=config::maxlowlevel)
    {
      delete file[0];
      file[0] = nullptr; 
    }
    else
      for(int i=0;i<ec_m;i++)
      {
        delete file[i];
        file[i] = nullptr;
      }
    long long BT_end_time = getCurrentTime();
    long long BT_all_time = BT_end_time - BT_start_time;
    long long Finish_all_time = Finish_end_time - Finish_start_time;
    total_times["BuildTable"] += BT_all_time;
    total_times["Finish"] += Finish_all_time;
    long long afterbt_start_time = getCurrentTime();
    if (s.ok()) {
      // Verify that the table is usable
      leveldb::ReadOptions opt = ReadOptions();
      opt.level = options.level;
      Iterator* it = table_cache->NewIterator(opt, meta->number,
                                              meta->file_size, level);
      s = it->status();
      delete it;
    }
    long long afterbt_end_time = getCurrentTime();
    total_times["afterbt"] += afterbt_end_time - afterbt_start_time;
  }

  // Check for input iterator errors
  if (!iter->status().ok()) {
    s = iter->status();
  }

  if (s.ok() && meta->file_size > 0) {
    // Keep it
    #ifdef STRIPE_RECORDER
    zal_utils::table_info build_table_info(meta->number, meta->smallest.user_key().ToString(), meta->largest.user_key().ToString(), meta->file_size);
    if (!table_info_set.contains(build_table_info)) {
      table_info_set.insert(build_table_info);
    }
    #endif
  } else {
    if(level<=config::maxlowlevel)
      env->RemoveFile(fname[0]);
      else
      for(int i=0;i<ec_m;i++)
        env->RemoveFile(fname[i]);
  }
  return s;
}

}  // namespace leveldb
