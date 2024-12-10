#ifndef YCSB_C_DUMMY_DB_H_
#define YCSB_C_DUMMY_DB_H_

#include <fstream>
#include <mutex>
#include <chrono>
#include <vector>
#include <unordered_set>

#include "core/db.h"
#include "utils/properties.h"

namespace ycsbc {

class DummyDB : public DB {
public:
    DummyDB() {}
    ~DummyDB() {}

    void Init();

    void Cleanup();

    Status Read(const std::string &table, const std::string &key,
                const std::vector<std::string> *fields, std::vector<Field> &result);

    Status Scan(const std::string &table, const std::string &key, int len, 
                const std::vector<std::string> *fields, std::vector<std::vector<Field>> &result);

    Status Update(const std::string &table, const std::string &key, std::vector<Field> &values);

    Status Insert(const std::string &table, const std::string &key, std::vector<Field> &values);

    Status Delete(const std::string &table, const std::string &key);

private:
    /*
      the generated file will be in the following format:
      operation, key, value
    */ 

    // Put and Get do not return anythin, because they cannot fail
    // DummyDB Put function to insert key-value pair into the "db", aka the file
    void db_put(const std::string& key, const std::string& value);
    // DummyDB Get function to get value from the "db", aka the file    
    void db_get(const std::string& key);
    // DummyDB delete function to delete a kv pair
    void db_delete(const std::string& key);

    void SerializeRow(const std::vector<Field> &values, std::string *data);
    void DeserializeRowFilter(std::vector<Field> *values, const std::string &data, const std::vector<std::string> &fields);
    void DeserializeRow(std::vector<Field> *values, const std::string &data);
    std::string BuildCompKey(const std::string &key, const std::string &field_name);
    std::string KeyFromCompKey(const std::string &comp_key);
    std::string FieldFromCompKey(const std::string &comp_key);

private:
    static std::ofstream* file_;    

    int fieldcount_;
    std::string field_prefix_;

    static size_t ref_cnt_;
    static std::mutex mu_;
};
}

#endif