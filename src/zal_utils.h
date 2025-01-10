#ifndef ZAL_UTILS_H
#define ZAL_UTILS_H

#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <string>
#include <map>
#include <unordered_set>
#include <chrono>
#include <leveldb/db.h>
#include <random>

namespace zal_utils {
/// gen_kv
std::string gen_value(std::mt19937& rng, size_t len);
std::string gen_key(size_t index);
std::string gen_key(size_t index, size_t len);

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

    bool nearly_full() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return queue_.size() >= capacity_ * 0.9;
    }

private:
    std::queue<T> queue_;
    mutable std::shared_mutex mutex_;
    std::condition_variable_any cond_empty_;
    std::condition_variable_any cond_full_;
    size_t capacity_;
};

/// thread_safe_set
template <typename T>
class ThreadSafeSet {
public:
    ThreadSafeSet() = default;
    ~ThreadSafeSet() = default;

    void insert(const T& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        set_.insert(value);
    }

    void erase(const T& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        set_.erase(value);
    }

    bool contains(const T& value) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return set_.find(value) != set_.end();
    }

    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return set_.empty();
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return set_.size();
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        set_.clear();
    }

    std::optional<T> find(const T& value) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = set_.find(value);
        if (it != set_.end()) {
            return *it;
        }
        return std::nullopt; // 返回空值表示未找到
    }

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        Iterator(typename std::unordered_set<T>::const_iterator it) : it_(it) {}

        reference operator*() const { return *it_; }
        pointer operator->() const { return &(*it_); }

        Iterator& operator++() {
            ++it_;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++it_;
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) { return a.it_ == b.it_; }
        friend bool operator!=(const Iterator& a, const Iterator& b) { return a.it_ != b.it_; }

    private:
        typename std::unordered_set<T>::const_iterator it_;
    };

    // 返回迭代器的begin和end方法
    Iterator begin() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return Iterator(set_.begin());
    }

    Iterator end() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return Iterator(set_.end());
    }

private:
    std::unordered_set<T> set_;
    mutable std::shared_mutex mutex_;
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
    FunctionTimer(const FunctionTimer* parent, const std::string& process_name);
    ~FunctionTimer();
    static void printTotalTimes();
    static void clearMap();

private:
    static std::map<std::string, long long> total_time;
    std::string function_name_;
    // static std::map<std::string, size_t> called_times;  // how many times has the same function been called.
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

struct table_info {
    int index;
    int level;
    int stripe_id;
    std::string smallest_key;
    std::string largest_key;
    size_t table_size;
    bool expired = false;

    table_info() = default;
    table_info(int index) : index(index), level(-1), stripe_id(-1), table_size(-1), expired(false) {}
    table_info(int index, size_t size) : index(index), level(-1), table_size(size), stripe_id(-1), expired(false) {}
    table_info(int index, int level, const std::string& smallest, const std::string& largest, size_t size) : index(index), level(level), smallest_key(smallest), largest_key(largest), table_size(size), stripe_id(-1), expired(false) {}
    table_info(int index, const std::string& smallest, const std::string& largest, size_t size) : index(index), level(-1), smallest_key(smallest), largest_key(largest), table_size(size), stripe_id(-1), expired(false) {}
    table_info(const table_info& other) = default;

    table_info& operator=(const table_info& other) = default;

    bool operator<(const table_info& other) const {
        return index < other.index;
    }

    bool operator==(const table_info& other) const {
        // since lzy did not store `largest_key` and `smallest_key` in every `FileMetaData`, we can not compare them
        return index == other.index
            // && smallest_key == other.smallest_key
            // && largest_key == other.largest_key
            // && table_size == other.table_size
            // && stripe_id == other.stripe_id
            // && disk_id == other.disk_id
            ;
    }

    void print() const {
        // if (level == -1) {
        //     std::cout << "table " << index << " range: " << smallest_key << " - " << largest_key << " size: " << table_size << std::endl;
        //     return;
        // }
        // else {
        //     std::cout << "table " << index << " level " << level << " range: " << smallest_key << " - " << largest_key << " size: " << table_size << std::endl;
        // }
        if (stripe_id != -1) {
            std::cout << "table: " << index << " stripe: " << stripe_id << std::endl;
        }
    }
};

struct compaction_info {
    std::vector<table_info> source;  // tables to be compacted
    std::vector<table_info> target;
    size_t index;

    bool operator<(const compaction_info& other) const {
        return index < other.index;
    }

    void print() const {
        for(int i=0;i<source.size();i++) {
            std::cout << source[i].index << "@" << source[i].level;
            if (i != source.size() - 1) {
                std::cout << " + ";
            }
        }
        std::cout << " ==> ";
        for(int i=0;i<target.size();i++) {
            std::cout << target[i].index << "@" << target[i].level;
            if (i != target.size() - 1) {
                std::cout << " & ";
            }
        }
        std::cout << std::endl;
    }
};

struct StripeRecorder {
    int id;
    std::vector<int> tables;  // vector to store the index of the sst once in the stripe

    StripeRecorder() = default;
    StripeRecorder(int id) : id(id) {}

    bool operator==(const StripeRecorder& other) const {
        if (this->id != other.id) {
            return false;
        }
        // as long as tables are the same, we think they are the same stripe
        for (int i = 0; i < this->tables.size() < other.tables.size() ? this->tables.size() : other.tables.size(); i++) {
            if (this->tables[i] != other.tables[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator<(const StripeRecorder& other) const {
        return id < other.id;
    }

    void print() const {
        std::cout << "stripe: " << id << " tables_id: ";
        for (int i = 0; i < tables.size(); i++) {
            std::cout << tables[i];
            if (i != tables.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }
};
} // namespace zal_utils

// reload std::hash for `table_info` and `StripeRecorder`
namespace std {
template <>
struct hash<zal_utils::StripeRecorder> {
    size_t operator()(const zal_utils::StripeRecorder& x) const {
        size_t h = 0;
        for (auto i : x.tables) {
            h ^= std::hash<int>()(i);
        }
        h ^= std::hash<int>()(x.id);
        return h;
    }
};
template <>
struct hash<zal_utils::table_info> {
    size_t operator()(const zal_utils::table_info &t) const {
        // since lzy is not storing `smallest_key` and `largest_key` in every `FileMetaData`, we can not use them to calculate hash
        std::size_t h1 = std::hash<int>()(t.index);
        // std::size_t h2 = std::hash<std::string>()(t.smallest_key);
        // std::size_t h3 = std::hash<std::string>()(t.largest_key);
        // std::size_t h4 = std::hash<size_t>()(t.table_size);
        // return h1 ^ (h2 << 1) ^ (h3 << 2) ;
        // size_t h5 = hash<int>()(t.stripe_id);
        return h1;
    }
};
}
#endif