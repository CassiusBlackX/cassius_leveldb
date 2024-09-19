#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>

#include <leveldb/db.h>

#include "zal_utils.h"

static const size_t KEYS = 1e6;
static const size_t READS = 1e3;

int main(){
    std::mt19937 rng(44);
    std::vector<std::string> all_values;
    for(size_t i=0;i<KEYS;i++) {
        all_values.push_back(zal_utils::gen_value(rng, 16));
    }
    
    const std::string dbName = "benchmarkdb";

    // 先把之前一次时候的数据库删除
    leveldb::Options opts;
	leveldb::DestroyDB(dbName, opts);
    // 创建并打开LevelDB数据库
    leveldb::DB* db;
    leveldb::Options options;
    leveldb::WriteOptions write_options;
    leveldb::ReadOptions read_options;
    options.create_if_missing = true;
    options.write_buffer_size = 2 * 1024 * 1024; // 2MB

    leveldb::Status status = leveldb::DB::Open(options, dbName, &db);

    zal_utils::FunctionTimer* main_add_keys_timer = new zal_utils::FunctionTimer("main_add_keys");
    for(size_t i=0;i<KEYS;i++) {
        const std::string& key = zal_utils::gen_key(i);
        const std::string& value = all_values[i];
        status = db->Put(write_options, key, value);
    }
    delete main_add_keys_timer;

    zal_utils::FunctionTimer* main_read_timer = new zal_utils::FunctionTimer("main_read");
    std::uniform_int_distribution<int> dist(0,KEYS-1);
    for(size_t i=0;i<READS;i++) {
        size_t index = dist(rng);
        const std::string& key = zal_utils::gen_key(index);
        std::string value;
        status = db->Get(read_options, key, &value);
        if (all_values[index] != value) {
            std::cerr << "key= " << key << "got incorrect value, expected: " << all_values[index] << " get: " << value << std::endl;
        } 
    }
    delete main_read_timer;
    delete db;

    zal_utils::FunctionTimer::printTotalTimes();
    return 0;
}