#include <vector>
#include <random>
#include <chrono>

#include <leveldb/db.h>

#include "zal_utils.h"

static constexpr size_t VALID_KEYS_COUNT = 1e5;
static constexpr size_t ITERATIONS = 1e6;
// key 16bytes, val 16bytes, total written 1e6 * 32bytes ~=~ 32MB

size_t compaction_info_index = 0;

zal_utils::ThreadSafeQueue<zal_utils::table_info> build_table_queue(800);
zal_utils::ThreadSafeQueue<zal_utils::compaction_info> compaction_info_queue(800);
std::vector<zal_utils::table_info> build_tables;
std::vector<zal_utils::compaction_info> compaction_infos;

int main() {
    std::mt19937 rng(44);
    std::uniform_int_distribution<int> dist(0, VALID_KEYS_COUNT-1);

    const std::string dbName = "testdb";
    leveldb::Options options;
    leveldb::DestroyDB(dbName, options);
    leveldb::DB* db;
    options.create_if_missing = true;
    options.write_buffer_size = 2 * 1024 * 1024; // 2MB
    leveldb::WriteOptions write_options;
    leveldb::Status status = leveldb::DB::Open(options, dbName, &db);

    for(size_t i=0; i<ITERATIONS; i++) {
    // const std::string& key = zal_utils::gen_key(dist(rng), 16);
        const std::string& key = zal_utils::gen_key(dist(rng));
        const std::string& value = zal_utils::gen_value(rng, 16);
        status = db->Put(write_options, key, value);
        if (!status.ok()) {
            std::cerr << "Failed to write key=" << key << std::endl;
            return 1;
        }
        if (build_table_queue.nearly_full()) {
            std::vector<zal_utils::table_info> messages;
            build_table_queue.pop_all(messages);
            for(const auto& message : messages) {
                build_tables.push_back(message);
            }
        }
        if (compaction_info_queue.nearly_full()) {
            std::vector<zal_utils::compaction_info> messages;
            compaction_info_queue.pop_all(messages);
            for(const auto& message : messages) {
                compaction_infos.push_back(message);
            }
        }
    }

    // sleep for a while to wait for compaction
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!build_table_queue.empty()) {
        std::vector<zal_utils::table_info> messages;
        build_table_queue.pop_all(messages);
        for(const auto& message : messages) {
            build_tables.push_back(message);
        }
    }
    if (!compaction_info_queue.empty()) {
        std::vector<zal_utils::compaction_info> messages;
        compaction_info_queue.pop_all(messages);
        for(const auto& message : messages) {
            compaction_infos.push_back(message);
        }
        sort(compaction_infos.begin(), compaction_infos.end());
    }
    std::cout << "build table infos: " << std::endl;
    for (const auto& table : build_tables) {
        table.print();
    }
    std::cout << std::endl;
    std::cout << "compaction infos: " << std::endl;
    for (const auto& compaction_info : compaction_infos) {
        compaction_info.print();
    }
    return 0;

}

// 似乎不是所有的.ldb文件都是sst文件！所以大概率目前的确已经找到所有的sst文件了