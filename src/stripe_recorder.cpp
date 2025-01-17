#include <iostream>
#include <string>
#include <thread>
#include <future>
#include <chrono>
#include <iomanip>
#include <vector>

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

zal_utils::ThreadSafeSet<zal_utils::StripeRecorder> stripe_recorder_set;
int global_stripe_index = 0;
std::mutex global_stripe_index_mutex;
zal_utils::ThreadSafeSet<zal_utils::table_info> table_info_set;

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
    props.SetProperty("threadcount", "2");
    props.SetProperty("dbname", "leveldb");
    props.SetProperty("status", "true");
    props.SetProperty("sleepafterload", "0");
    props.SetProperty("status.interval", "10");

    // workload
    const std::string& workload_name = std::string(CMAKELISTS_PATH) + "/ycsb/workloads/workload_ssd";
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

        std::cout << "Load runtime(sec): " << runtime << std::endl;
        std::cout << "Load operations(ops): " << sum << std::endl;
        std::cout << "Load throughput(ops/sec): " << sum / runtime << std::endl;
    }
    measurements->Reset();
    std::this_thread::sleep_for(std::chrono::seconds(std::stoi(props.GetProperty("sleepafterload", "0"))));

    std::cout << "after load, table_info_set size: " << table_info_set.size() << std::endl;

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

        std::cout << "Run runtime(sec): " << runtime << std::endl;
        std::cout << "Run operations(ops): " << sum << std::endl;
        std::cout << "Run throughput(ops/sec): " << sum / runtime << std::endl;    
    }
    for (int i=0; i<num_threads; i++) {
        delete dbs[i];
    }

    std::cout << "after transaction, table_info_set size: " << table_info_set.size() << std::endl;

    // handle info collected during runtime
    std::vector<zal_utils::table_info> tables_info;
    std::vector<zal_utils::StripeRecorder> stripe_recorders;
    for (const auto& table : table_info_set) {
        tables_info.push_back(table);
    }
    std::cout << "size of tables_info: " << tables_info.size() << std::endl;
    for (const auto& stripe : stripe_recorder_set) {
        stripe_recorders.push_back(stripe);
    }
    sort(tables_info.begin(), tables_info.end());
    sort(stripe_recorders.begin(), stripe_recorders.end());

    std::cout << "tables info: " << std::endl;
    for (const auto& table : tables_info) {
        table.print();
    }
    std::cout << "stripes info: " << std::endl;
    for (const auto& stripe : stripe_recorders) {
        stripe.print();
    }
    return 0;
}

// done: 数据集. done stripe_id唯一, 延迟删除(不删除)

/*
stripe_info {
    stripe_id: int
    tables: [int table_id]
    expired_count: int
}

table_info {
    table_id: int
    stripe_id: int
    valid: bool
}

table_info_set: ()
stripe_info_set: ()

当一个table要被删除的时候(compaction之后), 不删除,只在table_info_set中把这个table标记为失效 -> O(1)
现在还不能记录table对应的stripe_id, 只能遍历stripe_info_set,找到包含这个table的stripe,然后在stripe中把这个table标记为失效 -> O(n)
否则可以立刻去stripe_info_set中找到这个table对应的stripe, 然后在stripe中把这个table标记为失效 -> O(1)
如果stripe中失效table的数量超过阈值,就立刻删除这个stripe

当在查询的时候,查找每一个表之前,在table_info_set中查找这个表是否失效,失效就立刻跳过 -> O(1)

实际的删除stripe就是把失效的table删除,然后删除本条带对应的parity block, 然后在stripe_info_set中删除这个stripe



BUG 现在在记录table的时候没有办法正确记录到table对应的stripe_id

现在只能同时维护table_info_set和stripe_info_set

在tableinfo中标记sst的失效位,然后在每次调用RemoveObsoleteFiles的时候,遍历table_info_set和stripe_info_set,判断是否要删除一个条带  -> O(n) 目前版本

还有一个思路是在每次标记失效位之后就立刻更新stripe_info_set, 然后超过阈值就立刻调用删除stripe  -> 未实现

在get方法中,查找table之前,进table_info_set检查该table的失效位 O(1)

*/