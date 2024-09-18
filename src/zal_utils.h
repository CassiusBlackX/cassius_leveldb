#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <string>
#include <map>
#include <chrono>
#include <leveldb/db.h>
#include <random>

namespace zal_utils {
/// gen_kv
std::string gen_value(std::mt19937& rng);
std::string gen_key(size_t index);

/// thread_safe_queue
template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const size_t capacity) : capacity_(capacity) {}
    ~ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void push(T value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        if (queue_.size() >= capacity_) {
            std::cout << "queue is full, waiting..." << std::endl;
        }
        cond_full_.wait(lock, [this] { return queue_.size() < capacity_; });
        queue_.push(std::move(value));
        cond_empty_.notify_one();
    }

    void pop_all(std::vector<T>& values) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        cond_empty_.wait(lock, [this] { return !queue_.empty(); });
        while (!queue_.empty()) {
            values.push_back(queue_.front());
            queue_.pop();
        }
        cond_full_.notify_one();
    }

    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return queue_.empty();
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    mutable std::shared_mutex mutex_;
    std::condition_variable_any cond_empty_;
    std::condition_variable_any cond_full_;
    size_t capacity_;
};

/// custom snapshot
class CSnapshot {
public:
    explicit CSnapshot(leveldb::DB* db) : db_(db), snapshot_(db->GetSnapshot()) {}
    
    ~CSnapshot() {
        db_->ReleaseSnapshot(snapshot_);
    }

private:
    leveldb::DB* db_;
    const leveldb::Snapshot* snapshot_;
};

// timer
class FunctionTimer{
public:
    FunctionTimer(const std::string& function_name);
    FunctionTimer(const FunctionTimer& parent, const std::string& process_name);
    ~FunctionTimer();
    static void printTotalTimes();
    static void clearMap();

private:
    static std::map<std::string, long long> total_times;
    std::string function_name_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

// path_string
/// @brief replace the number in the `pathStr` with the given `disknumber`
std::string replaceDiskNumber(const std::string& pathStr, unsigned diskNumber);

/// @brief replace the number in the `pathStr` with the given `diskNumber` but only at the `rindex`th part
/// @param pathStr 
/// @param diskNumber 
/// @param rindex usually, it is negative, which means the index from the end. e.g. -1 means the last part
/// @return replaced pathString
std::string replaceDiskNumber(const std::string& pathStr, unsigned diskNumber, int rindex);

struct table_range {
    unsigned index;
    std::string smallest;
    std::string largest;

    table_range(unsigned index, const std::string& smallest, const std::string& largest) : index(index), smallest(smallest), largest(largest) {}
    table_range() = default;

};
} // namespace zal_utils

#define ec_m 6
#define ec_k 4
#define ec_p (ec_m - ec_k)