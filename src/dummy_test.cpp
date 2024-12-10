#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <string>
#include <sstream>
#include <ctime>
#include <iterator>
#include <chrono>
#include <map>

#include <leveldb/db.h>

#include "zal_utils.h"

static constexpr size_t OPERATIONS_COUNT = 1E6;

int main() {
    std::mt19937 rng(44);
    std::uniform_int_distribution<int> dist(0, OPERATIONS_COUNT-1);

    const std::string dbName = "testdb";
    leveldb::Options options;
    leveldb::DestroyDB(dbName, options);

    leveldb::DB* db;
    options.create_if_missing = true;
    options.write_buffer_size = 32 * 1024 * 1024;  // 32MB
    options.max_file_size = 32 * 1024 * 1024;

    leveldb::WriteOptions write_options;
    leveldb::ReadOptions read_options;

    leveldb::Status status = leveldb::DB::Open(options, dbName, &db);

    for(size_t i=0; i<OPERATIONS_COUNT; i++) {
        const std::string& key = zal_utils::gen_key(i, 16);
        const std::string& value = zal_utils::gen_value(rng, 16);
        status = db->Put(write_options, key, value);
        if (!status.ok()) {
            std::cerr << "failed to write key=" << key << std::endl;
            return 1;
        }
    }

    for (size_t i=0; i<OPERATIONS_COUNT; i++) {
        const std::string& key = zal_utils::gen_key(dist(rng), 16);
        const std::string& value = zal_utils::gen_value(rng, 16);
        status = db->Put(write_options, key, value);
        if (!status.ok()) {
            std::cerr << "failed to write key=" << key << std::endl;
            return 1;
        }
    }

    std::cout << "64MB data written" << std::endl;
    return 0;
}