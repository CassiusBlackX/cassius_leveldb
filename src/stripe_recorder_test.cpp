#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/status.h"
#include "leveldb/replicalog.h"

#include "zal_utils.h"

static const size_t VALID_KEYS_COUNT = 1e5;
static const double MODIFY_RATIO = 0.35;
static const double READ_RATIO = 0.55;
static const size_t RECENT = 1e5;
static const size_t ITERATIONS = 1e3;
static const size_t VALUE_LEN = 16;

int main() {
  std::mt19937 rng(44);
  std::uniform_int_distribution<int> dist(0, VALID_KEYS_COUNT - 1);

  const std::string db_name = "lzydb";
  leveldb::Options opts;
  leveldb::DestroyDB(db_name, opts);

  leveldb::DB* db;
  leveldb::WriteOptions write_options;
  leveldb::ReadOptions read_options;
  opts.create_if_missing = true;
  opts.write_buffer_size = 4 * 1024 * 1024;

  // lzy's stuff
  const std::string log_path1 = std::string(CMAKELISTS_PATH) + "/build/testdb/data0";
  const std::string log_path2 = std::string(CMAKELISTS_PATH) + "/build/testdb/data0;

  const std::string ec_path0 = std::string(CMAKELISTS_PATH) + "/build/testdb/data0";
  const std::string ec_path1 = std::string(CMAKELISTS_PATH) + "/build/testdb/data1";
  const std::string ec_path2 = std::string(CMAKELISTS_PATH) + "/build/testdb/data2";
  const std::string ec_path3 = std::string(CMAKELISTS_PATH) + "/build/testdb/data3";
  const std::string ec_path4 = std::string(CMAKELISTS_PATH) + "/build/testdb/parity0";
  const std::string ec_path5 = std::string(CMAKELISTS_PATH) + "/build/testdb/parity1";

  const int replicaNum=2;
  const int ecNum = 6;

  std::vector<std::string> replicapath;

  replicapath.push_back(log_path1);
  replicapath.push_back(log_path2);

  leveldb::ReplicaLog replicalog;

  replicalog.setReplicaMeta(replicaNum,replicapath);

  std::vector<std::string> ecpath;

  ecpath.push_back(ec_path0);
  ecpath.push_back(ec_path1);
  ecpath.push_back(ec_path2);
  ecpath.push_back(ec_path3);
  ecpath.push_back(ec_path4);
  ecpath.push_back(ec_path5);

  leveldb::Ecpath Aecpath;

  Aecpath.setEcpathMeta(ecNum,ecpath);

  leveldb::Status status = leveldb::DB::Open(opts, db_name, &db, replicalog, Aecpath);

  if (!status.ok()) {
    std::cerr << "failed to open db!" << std::endl;
    return -1;
  }
  // maintain a map tof the correect kv
  std::unordered_map<std::string, std::string> store;

  for (size_t i = 0; i < VALID_KEYS_COUNT; i++) {
    const std::string& key = zal_utils::gen_key(i);
    const std::string& value = zal_utils::gen_value(rng, VALUE_LEN);
    store[key] = value;
    status = db->Put(write_options, key, value);
    if (!status.ok()) {
      std::cerr << "failed to write key= " << key << std::endl;
      return 1;
    }
  }
  std::cout << "success initial put" << std::endl;

  for (size_t i = 0; i < ITERATIONS; i++) {
    static const size_t modifies = VALID_KEYS_COUNT * MODIFY_RATIO;
    static const size_t reads = VALID_KEYS_COUNT * READ_RATIO;

    // modify
    for (size_t j = 0; j < modifies; j++) {
      const size_t index = dist(rng);
      const std::string& key = zal_utils::gen_key(index);
      const std::string& value = zal_utils::gen_value(rng, VALUE_LEN);
      store[key] = value;
      status = db->Put(write_options, key, value);
      if (!status.ok()) {
        std::cerr << "failed to write key= " << key << std::endl;
        return 1;
      }
    }

    // read
    for (size_t j = 0; j < reads; j++) {
      const size_t index = dist(rng);
      const std::string& key = zal_utils::gen_key(index);
      std::string value;
      status = db->Get(read_options, key, &value);
      if (status.IsNotFound()) {
        std::cerr << "Value MISSING key= " << key << std::endl;
        return 1;
      }
      if (!status.ok()) {
        std::cerr << "failed to read key= " << key << std::endl;
        return 1;
      }
      if (value != store[key]) {
        std::cerr << "valie MisMatch key= " << key
                  << " expected value= " << store[key]
                  << " got value= " << value << std::endl;
        return 1;
      }
    }
    std::cout << "iteration: " << i << std::endl;
  }

  std::cout << "all right!" << std::endl;
}
