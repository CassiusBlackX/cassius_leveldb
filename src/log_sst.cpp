#include <vector>
#include <random>
#include <chrono>
#include <unordered_set>

#include <leveldb/db.h>

#include "zal_utils.h"

static constexpr size_t VALID_KEYS_COUNT = 1e6;
static constexpr size_t ITERATIONS = 1e6;
// key 16bytes, val 16bytes, total written 1e6 * 32bytes ~=~ 32MB


/*
所有调用builder->Finish()的地方都是在生成sst，所以必须要在每个builder->Finish()的地方记录sst的信息。
但是在builder->Finish()中，无法记录sst的level，所以只能在其它地方记录level。
*/
zal_utils::ThreadSafeQueue<zal_utils::table_info> build_table_queue(800);
zal_utils::ThreadSafeQueue<zal_utils::compaction_info> compaction_info_queue(800);
std::vector<zal_utils::table_info> build_tables;
std::vector<zal_utils::compaction_info> compaction_infos;

namespace std {
template <>
struct hash<zal_utils::table_info> {
    std::size_t operator()(const zal_utils::table_info &t) const {
            std::size_t h1 = std::hash<unsigned>()(t.index);
            std::size_t h2 = std::hash<std::string>()(t.smallest_key);
            std::size_t h3 = std::hash<std::string>()(t.largest_key);
            // std::size_t h4 = std::hash<size_t>()(t.table_size);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ;
        }
    };
}

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
    std::this_thread::sleep_for(std::chrono::seconds(3));

/*
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
        for (const auto& table : compaction_info.source) {
            table.print();
        }
        for (const auto& table : compaction_info.target) {
            table.print();
        }
        std::cout << std::endl;
    }
*/
    std::unordered_set<zal_utils::table_info> built_table_set;
    std::vector<zal_utils::table_info> built_tables;
    std::vector<zal_utils::compaction_info> compacted_infos;

    if (!build_table_queue.empty()) {
        build_table_queue.pop_all(built_table_set);
    }
    
    if (!compaction_info_queue.empty()) {
        compaction_info_queue.pop_all(compacted_infos);
    }
    sort(compacted_infos.begin(), compacted_infos.end());

    std::cout << "compaction infos: " << std::endl;
    for (const auto & compaction_info : compacted_infos) {
        compaction_info.print();
        for (const auto& table : compaction_info.source) {
            auto it = built_table_set.find(table);
            if (it != built_table_set.end()) {
                zal_utils::table_info updated_table = *it;
                updated_table.level = table.level;
                built_table_set.erase(it);
                built_table_set.insert(updated_table);
            }
            else {
                built_table_set.insert(table);
            }
        }
        for (const auto& table : compaction_info.target) {
            auto it = built_table_set.find(table);
            if (it != built_table_set.end()) {
                zal_utils::table_info updated_table = *it;
                updated_table.level = table.level;
                built_table_set.erase(it);
                built_table_set.insert(updated_table);
            }
            else {
                built_table_set.insert(table);
            }
        }
    }
    
    for (const auto& table : built_table_set) {
        built_tables.push_back(table);
    }
    sort(built_tables.begin(), built_tables.end());
    for (const auto& table : built_tables) {
        table.print();
    }

    return 0;

}
