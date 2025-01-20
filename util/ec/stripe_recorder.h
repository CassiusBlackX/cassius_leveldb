#ifndef UTILS_EC_STRIPE_RECORDER_H_
#define UTILS_EC_STRIPE_RECORDER_H_
// zal added
#include <vector>
#include <unordered_map>

class StripeRecorder {
public:
    StripeRecorder() = default;
    ~StripeRecorder() = default;
    StripeRecorder(unsigned delete_threshold) : delete_threshold(delete_threshold) {}
    // when adding a table, also add the table to the StripeRecorder
    void AddTable(int table_id, int stripe_id);
    // when deleting a table, also delete the table from the StripeRecorder
    void DeleteTable(int table_id);    

private:
    unsigned delete_threshold;

    struct stripe_info {
        int stripe_id;
        std::vector<int> tables;
        int expired_count;
        stripe_info() = default;
        stripe_info(int stripe_id) : stripe_id(stripe_id), expired_count(0) {}
    };

    struct table_info {
        int table_id;
        int stripe_id;
        bool valid;
        table_info() = default;
        table_info(int table_id, int stripe_id) : table_id(table_id), stripe_id(stripe_id), valid(true) {}
    };

    std::unordered_map<int, table_info> table_infos;
    std::unordered_map<int, stripe_info> stripe_infos;
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

当一个table要被删除的时候(compaction之后), 不删除,只在table_info_set中把这个table标记为失效 -> O(1)
现在还不能记录table对应的stripe_id, 只能遍历stripe_info_set,找到包含这个table的stripe,然后在stripe中把这个table标记为失效 -> O(n)
否则可以立刻去stripe_info_set中找到这个table对应的stripe, 然后在stripe中把这个table标记为失效 -> O(1)
如果stripe中失效table的数量超过阈值,就立刻删除这个stripe

当在查询的时候,查找每一个表之前,在table_info_set中查找这个表是否失效,失效就立刻跳过 -> O(1)

实际的删除stripe就是把失效的table删除,然后删除本条带对应的parity block, 然后在stripe_info_set中删除这个stripe
*/