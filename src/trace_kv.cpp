#include <sstream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <regex>
#include <tuple>
#include <algorithm>
#include <vector>

#include <leveldb/db.h>

#include "zal_utils.h"

static const size_t VALID_KEYS_COUNT = 100000;
static const double MODIFY_RATIO = 0.35;
static const double READ_RATIO = 0.55;
static const size_t RECENT = 1e5;
static const size_t ITERATIONS = 1e3;

zal_utils::ThreadSafeQueue<std::tuple<std::string, size_t>> tsQueue_key_table(VALID_KEYS_COUNT+1); 
zal_utils::ThreadSafeQueue<std::tuple<std::string, size_t>> trace_4571(800);
zal_utils::ThreadSafeQueue<zal_utils::table_range> table_range(800);

std::string UnescapeString(const std::string& str) {
    // 这里假设 EscapeString 只是简单地转义了单引号
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size() && str[i + 1] == '\'') {
            result += '\'';
            ++i; // 跳过转义字符
        } else {
            result += str[i];
        }
    }
    return result;
}

std::deque<zal_utils::CSnapshot> snapshots;
std::unordered_map<std::string, std::queue<int>> key_table;


int main() {
    // BUG even when using random seed, the behavior of the functino is not stable!
    std::mt19937 rng(44);
    std::uniform_int_distribution<int> dist(0, VALID_KEYS_COUNT-1);

    std::string dbName = "testdb";
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
    
    // 维持一个有效kv的实际值map, 用于验证读取的正确性
    std::unordered_map<std::string, std::string> store;
    
    std::vector<zal_utils::table_range> table_ranges;

    // store all InternalKeys and their related values
    size_t global_sequnce = 0;  // in leveldb, for every key, adding to the only sequence number

    for(size_t i = 0; i < VALID_KEYS_COUNT; i++) {
        const std::string& key = zal_utils::gen_key(i);
        const std::string& value = zal_utils::gen_value(rng, 16);
        store[key] = value;
        status = db->Put(write_options, key, value);
        if (!status.ok()) {
            std::cerr << "Failed to write key=" << key << std::endl;
            return 1;
        }
    }

    // trigger flag
    bool triggered = false;

    for(size_t i = 0; i < ITERATIONS; i++) {
        static const size_t modifies = VALID_KEYS_COUNT * MODIFY_RATIO;
        static const size_t reads = VALID_KEYS_COUNT * READ_RATIO;

        // modify
        for(size_t j=0;j<modifies;j++) {
            const size_t index = dist(rng);
            const std::string& key = zal_utils::gen_key(index);
            const std::string& value = zal_utils::gen_value(rng, 16);
            store[key] = value;
            status = db->Put(write_options, key, value);
            if (!status.ok()) {
                std::cerr << "Failed to write key=" << key << std::endl;
                return 1;
            }

            if (!tsQueue_key_table.empty()) {
                std::vector<std::tuple<std::string, size_t>> messages;
                tsQueue_key_table.pop_all(messages);
                for (const auto& message : messages) {
                    const std::string& key = std::get<0>(message);
                    size_t sequence = std::get<1>(message);
                    key_table[key].push(sequence);
                }
            } 
        }

        // read
        for(size_t j=0;j<reads;j++) {
            if (!tsQueue_key_table.empty()) {
                std::vector<std::tuple<std::string, size_t>> messages;
                tsQueue_key_table.pop_all(messages);
                for (const auto& message : messages) {
                    const std::string& key = std::get<0>(message);
                    size_t sequence = std::get<1>(message);
                    key_table[key].push(sequence);
                }
            }

            if (!table_range.empty()) {
                std::vector<zal_utils::table_range> messages;
                table_range.pop_all(messages);
                for (const auto& message : messages) {
                    table_ranges.push_back(message);
                }
            }
             
            const size_t index = dist(rng);
            const std::string& key = zal_utils::gen_key(index);
            std::string value;
            status = db->Get(read_options, key, &value);
            if (status.IsNotFound()) {
                std::cerr << "Value MISSING key=" << key << std::endl;
                return 1;
            }
            if (!status.ok()) {
                std::cerr << "Failed to read key=" << key << std::endl;
                return 1;
            }
            if (value != store[key]) {
                std::cerr << "Value MISMATCH key= " << key << ", expected Value=" << store[key] << ", Get Value=" << value << std::endl;
                std::queue<int> target = key_table[key];
                std::cerr << "key=" << key << " was once stored in " << target.size() << " sstabls' boundary" << std::endl;
                for (auto it = target; !it.empty(); it.pop()) {
                    std::cerr << it.front() << " ";
                }
                std::cerr << std::endl;
                
                for (auto it = table_ranges.begin(); it != table_ranges.end(); it++) {
                    if (it->smallest <= key && key <= it->largest) {
                        std::cerr << "key=" << key << " was once stored in table " << it->index << std::endl;
                    }
                }
                return 2;
            }
        }
        std::cout << "Iteration " << i << " finished" <<std::endl;

        if(dist(rng)  < (VALID_KEYS_COUNT / 2)){
            snapshots.emplace_back(db);
        }
        if (triggered) {
            break;
        }
    }

    return 0;
}

// sst直接生成的就不一样
// sst是一开始一样的,但是compaction之后就不一样了
// 记录每个sst时候的key的范围