#ifndef YCSB_C_LEVELDB_DB_H_
#define YCSB_C_LEVELDB_DB_H_

#include <mutex>

#include "core/db.h"
#include "utils/properties.h"

#include <leveldb/db.h>

namespace ycsbc {

class LeveldbDB : public DB {
public:
    LeveldbDB();
    ~LeveldbDB();

    void Init() override;

    void Cleanup() override;

    Status Read(const std::string& table, const std::string& key,
                const std::vector<std::string> *fields,
                std::vector<Field>& result) override;

    Status Scan(const std::string& table, const std::string& key,
                int record_count, const std::vector<std::string> *fields,
                std::vector<std::vector<Field>> &result) override;

    Status Update(const std::string& table, const std::string& key,
                  std::vector<Field>& values) override;

    Status Insert(const std::string& table, const std::string& key,
                    std::vector<Field>& values) override;

    Status Delete(const std::string& table, const std::string& key) override;

private:
    enum format {
        kSingleEntry,  // read and write kv one by one
        kRowMajor,  // read and write kv in row major
        kColumnMajor  // read and write kv in column major
    };

    format format_;

    int field_count_;
    std::string field_prefix_;

    static leveldb::DB *db_;
    static int ref_cnt_;
    static std::mutex mu_;

    void GetOptions(const utils::Properties& props, leveldb::Options& options);
    void SerializeRow(const std::vector<Field>& values, std::string& data);
    void DeserializeRowFilter(std::vector<Field>& values, const std::string& data, const std::vector<std::string>& fields);
    void DeserializeRow(std::vector<Field>& values, const std::string& data);
    std::string BuildCompKey(const std::string& key, const std::string& field_name);
    std::string KeyFromCompKey(const std::string& comp_key);
    std::string FieldFromCompKey(const std::string& comp_key);

    Status ReadSingleEntry(const std::string& table, const std::string& key, const std::vector<std::string>* fields, std::vector<Field>& result);
    Status ScanSingleEntry(const std::string& table, const std::string& key, int len, const std::vector<std::string>* fields, std::vector<std::vector<Field>>& result);
    Status UpdateSingleEntry(const std::string& table, const std::string& key, std::vector<Field>& values);
    Status InsertSingleEntry(const std::string& table, const std::string& key, std::vector<Field>& values);
    Status DeleteSingleEntry(const std::string& table, const std::string& key);

    Status ReadCompKeyRM(const std::string& table, const std::string& key, const std::vector<std::string>* fields, std::vector<Field>& result);
    Status ScanCompKeyRM(const std::string& table, const std::string& key, int len, const std::vector<std::string>* fields, std::vector<std::vector<Field>>& result);
    Status ReadCompKeyCM(const std::string& table, const std::string& key, const std::vector<std::string>* fields, std::vector<Field>& result);
    Status ScanCompKeyCM(const std::string& table, const std::string& key, int len, const std::vector<std::string>* fields, std::vector<std::vector<Field>>& result);
    Status InsertCompKey(const std::string& table, const std::string& key, std::vector<Field>& values);
    Status DeleteCompKey(const std::string& table, const std::string& key);

    Status (LeveldbDB::*method_read_)(const std::string&, const std::string&, const std::vector<std::string>*, std::vector<Field>&);
    Status (LeveldbDB::*method_scan_)(const std::string&, const std::string&, int, const std::vector<std::string>*, std::vector<std::vector<Field>>&);
    Status (LeveldbDB::*method_update_)(const std::string&, const std::string&, std::vector<Field>&);
    Status (LeveldbDB::*method_insert_)(const std::string&, const std::string&, std::vector<Field>&);
    Status (LeveldbDB::*method_delete_)(const std::string&, const std::string&);
};  // class LeveldbDB

DB* NewleveldbDB();

}  // namespace ycsbc

#endif  // YCSB_C_LEVELDB_DB_H_