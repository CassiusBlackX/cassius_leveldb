#ifndef UTILS_EC_STRIPE_RECORDER_H_
#define UTILS_EC_STRIPE_RECORDER_H_
// zal added
#include <unordered_map>
#include <queue>
#include <mutex>

class StripeRecorder {
 public:
  StripeRecorder() = default;
  ~StripeRecorder() = default;
  StripeRecorder(unsigned delete_threshold)
      : delete_threshold(delete_threshold) {}
  // when adding a table, also add the table to the StripeRecorder
  void AddTable(int table_id, int stripe_id);
  // when leveldb tries to delete a table, a table is marked as expired, but will not be deleted immediately
  void ExpireTable(int table_id);
  // when there are tables in `do_be_deleted`, use `env` funcs to physically delete the tables
  void DeleteTable(std::vector<std::string>& files_names, const std::string& dbname);
  // when looking up a table, check if the table is still valid in the stripe, if valid, return true
  bool LookUpTable(int table_id) const;

 private:
  unsigned delete_threshold;
  std::mutex s_mutex_;
  std::mutex t_mutex_;
  std::mutex q_mutex_;


  struct stripe_info {
    int stripe_id;
    std::vector<int> tables;
    int expired_count;
    stripe_info() = default;
    stripe_info(int stripe_id) : stripe_id(stripe_id), expired_count(0) {
      tables.reserve(4);
    }
  };

  struct table_info {
    int table_id;
    int stripe_id;
    bool valid;
    table_info() = default;
    table_info(int table_id, int stripe_id)
        : table_id(table_id), stripe_id(stripe_id), valid(true) {}
  };

  std::unordered_map<int, table_info> table_infos;
  std::unordered_map<int, stripe_info> stripe_infos;
  std::queue<int> to_be_deleted_tables;  // would be convenient when we are going to physically delete the tables from the disk
};

#endif  // UTILS_EC_STRIPE_RECORDER_H_

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

当一个table要被删除的时候(compaction之后),
不删除,只在table_info_set中把这个table标记为失效 -> O(1)
然后在stripe中把这个table标记为失效 -> O(1)
如果stripe中失效table的数量超过阈值,就立刻删除这个stripe

当在查询的时候,查找每一个表之前,在table_info_set中查找这个表是否失效,失效就立刻跳过
-> O(1)

实际的删除stripe就是把失效的table删除,然后删除本条带对应的parity block,
然后在stripe_info_set中删除这个stripe
*/
