#include <iostream>
#include <string>
#include <thread>
#include <future>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <unordered_set>

#include <leveldb/db.h>

#include "leveldbDB.h"
#include "core/client.h"
#include "core/core_workload.h"
#include "core/db_factory.h"
#include "core/measurements.h"
#include "utils/countdown_latch.h"
#include "utils/rate_limit.h"
#include "utils/timer.h"
#include "utils/utils.h"

#include "zal_utils.h"

#ifndef CMAKELISTS_PATH
#define CMAKELISTS_PATH "."
#endif

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

std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();
// we are not sure how many ssts are going to be generated, therefore we put all of them in the queue, until ycsb finishes
zal_utils::ThreadSafeQueue<zal_utils::table_info> build_table_queue(2e5);
zal_utils::ThreadSafeQueue<zal_utils::compaction_info> compaction_info_queue(2e5);

void StatusThread(ycsbc::Measurements* measurements, ycsbc::utils::CountDownLatch* latch, int interval) {
    using namespace std::chrono;
    time_point<system_clock> start = system_clock::now();
    bool done = false;
    while (true) {
        time_point<system_clock> now = system_clock::now();
        std::time_t now_c = system_clock::to_time_t(now);
        duration<double> elapsed_time = now - start;

        std::cout << std::put_time(std::localtime(&now_c), "%F %T") << ' '
                  << static_cast<long long>(elapsed_time.count()) << " sec: ";

        std::cout << measurements->GetStatusMsg() << std::endl;

        if (done) {
            break;
        }
        done = latch->AwaitFor(interval);
    }
}

void RateLimitThread(std::string rate_file, std::vector<ycsbc::utils::RateLimiter*> rate_limiters, ycsbc::utils::CountDownLatch* latch) {
    std::ifstream ifs(rate_file);
    if (!ifs.is_open()) {
        ycsbc::utils::Exception("failed to open: " + rate_file);
    }

    size_t num_threads = rate_limiters.size();
    size_t last_time = 0;
    while (!ifs.eof()) {
        size_t next_time;
        size_t next_rate;
        ifs >> next_time >> next_rate;

        if (next_time <= last_time) {
            ycsbc::utils::Exception("invalid rate file");
        }

        bool done = latch->AwaitFor(next_time - last_time);
        if (done) {
            break;
        }
        last_time = next_time;

        for (auto x : rate_limiters) {
            x->SetRate(next_rate / num_threads);
        }
    }
}

int main() {
    ycsbc::utils::Properties props;
    props.SetProperty("doload", "true");
    props.SetProperty("dotransaction", "true");
    props.SetProperty("threadcount", "20");
    props.SetProperty("dbname", "leveldb");
    props.SetProperty("status", "false");
    props.SetProperty("sleepafterload", "0");

    // workload
    const std::string& workload_name = std::string(CMAKELISTS_PATH) + "/ycsb/workloads/workload_benchmark.ini";
    std::ifstream input(workload_name);
    try {
        props.Load(input);
    } catch (const std::string& message) {
        std::cerr << message << std::endl;
        exit(0);
    }
    input.close();
    // db property
    const std::string& db_property = std::string(CMAKELISTS_PATH) + "/ycsb/properties/ssd.properties"; 
    std::ifstream db_input(db_property);
    try {
        props.Load(db_input);
    } catch (const std::string& message) {
        std::cerr << message << std::endl;
        exit(0);
    }
    db_input.close();

    const bool do_load = (props.GetProperty("doload", "false") == "true");
    const bool do_transaction = (props.GetProperty("dotransaction", "false") == "true");
    if (!do_load && !do_transaction) {
        std::cerr << "No operation to do" << std::endl;
        exit(1);
    }

    const int num_threads = std::stoi(props.GetProperty("threadcount", "1"));

    ycsbc::Measurements* measurements = ycsbc::CreateMeasurements(&props);
    if (measurements == nullptr) {
        std::cerr << "Unknown measurements name" << std::endl;
        exit(1);
    }

    std::vector<ycsbc::DB*> dbs;
    for (int i = 0; i < num_threads; i++) {
        ycsbc::DB* db = ycsbc::DBFactory::CreateDB(&props, measurements);
        if (db == nullptr) {
            std::cerr << "Unknown database name " << props["dbname"] << std::endl;
            exit(1);
        }
        dbs.push_back(db);
    }

    ycsbc::CoreWorkload wl;
    wl.Init(props);

    // print status periodically
    const bool show_status = (props.GetProperty("status", "false") == "true");
    const int status_interval = std::stoi(props.GetProperty("status.interval", "10"));

    // load phase
    if (do_load) {
        const int total_ops = std::stoi(props[ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY]);

        ycsbc::utils::CountDownLatch latch(num_threads);
        ycsbc::utils::Timer<double> timer;
        timer.Start();

        std::future<void> status_future;
        if (show_status) {
            status_future = std::async(std::launch::async, StatusThread, measurements, &latch, status_interval);
        }
        std::vector<std::future<int>> client_threads;
        for (int i = 0; i < num_threads; ++i) {
            int thread_ops = total_ops / num_threads;
            if (i < total_ops % num_threads) {
                thread_ops++;
            }

            client_threads.emplace_back(std::async(std::launch::async, ycsbc::ClientThread, dbs[i], &wl, thread_ops, true, true, !do_transaction, &latch, nullptr));
        }
        assert((int)client_threads.size() == num_threads);

        int sum = 0;
        for (auto &n : client_threads) {
            assert(n.valid());
            sum += n.get();
        }
        double runtime = timer.End();

        if (show_status) {
            status_future.wait();
        }

        // std::cout << "Load runtime(sec): " << runtime << std::endl;
        // std::cout << "Load operations(ops): " << sum << std::endl;
        // std::cout << "Load throughput(ops/sec): " << sum / runtime << std::endl;
    }
    measurements->Reset();
    std::this_thread::sleep_for(std::chrono::seconds(std::stoi(props.GetProperty("sleepafterload", "0"))));

    // transaction phase
    if (do_transaction) {
        // initial ops per second, unlimited if <= 0
        const int64_t ops_limit = std::stoi(props.GetProperty("limit.ops", "0"));
        // rate file path for dynamic rate limiting, format "time_stamp_sec new_ops_per_second" per line
        std::string rate_file = props.GetProperty("limit.file", "");

        const int total_ops = std::stoi(props[ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);

        ycsbc::utils::CountDownLatch latch(num_threads);
        ycsbc::utils::Timer<double> timer;
        timer.Start();

        std::future<void> status_future;
        if (show_status) {
            status_future = std::async(std::launch::async, StatusThread, measurements, &latch, status_interval);
        }
        std::vector<std::future<int>> client_threads;
        std::vector<ycsbc::utils::RateLimiter*> rate_limiters;
        for (int i = 0; i < num_threads; i++) {
            int thread_ops = total_ops / num_threads;
            if (i < total_ops % num_threads) {
                thread_ops++;
            }
            ycsbc::utils::RateLimiter* rlim = nullptr;
            if (ops_limit > 0 || rate_file != "") {
                int64_t per_thread_ops = ops_limit / num_threads;
                rlim = new ycsbc::utils::RateLimiter(per_thread_ops, per_thread_ops);
            }
            rate_limiters.push_back(rlim);
            client_threads.emplace_back(std::async(std::launch::async, ycsbc::ClientThread, dbs[i], &wl, thread_ops, false, !do_load, true, &latch, rlim));
        }

        std::future<void> rlim_future;
        if (rate_file != "") {
            rlim_future = std::async(std::launch::async, RateLimitThread, rate_file, rate_limiters, &latch);
        }

        assert((int)client_threads.size() == num_threads);

        int sum = 0;
        for (auto &n : client_threads) {
            assert(n.valid());
            sum += n.get();
        }
        double runtime = timer.End();

        if (show_status) {
            status_future.wait();
        }

        // std::cout << "Run runtime(sec): " << runtime << std::endl;
        // std::cout << "Run operations(ops): " << sum << std::endl;
        // std::cout << "Run throughput(ops/sec): " << sum / runtime << std::endl;    
    }
    for (int i=0; i<num_threads; i++) {
        delete dbs[i];
    }

    // sleep for a while to wait for compaction finishes
    std::this_thread::sleep_for(std::chrono::seconds(3));

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

    std::cout << "total amount of compaction infos: " << compacted_infos.size() << std::endl;
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
                // the new compacted generated table is not in built_table
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
                // the new compacted generated table is not in built_table
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