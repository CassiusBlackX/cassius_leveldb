#include <iostream>
#include <sstream>

#include "dummyDB.h"
#include "core/core_workload.h"
#include "core/db_factory.h"
#include "utils/utils.h"

#ifndef CMAKELISTS_PATH
#define CMAKELISTS_PATH "."
#endif


namespace ycsbc {

std::mutex DummyDB::mu_;
size_t DummyDB::ref_cnt_ = 0;
std::ofstream* DummyDB::file_ = nullptr;

// because the ycsb values is potentially to be binary instead of ASCII or UTF-8
// we need to use base64 to encode the value, then writing it into the csv file
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string Base64Encode(const std::string &in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::string Base64Decode(const std::string &in) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

void DummyDB::db_put(const std::string &key, const std::string &value) {
    const std::lock_guard<std::mutex> lock(mu_);
    *file_ << "put, " << key << ", " << Base64Encode(value) << std::endl;
}

void DummyDB::db_get(const std::string &key) {
    const std::lock_guard<std::mutex> lock(mu_);
    *file_ << "get, " << key << std::endl;  // Get method does not need a value
}

void DummyDB::db_delete(const std::string &key) {
    const std::lock_guard<std::mutex> lock(mu_);
    *file_ << "delete, " << key << std::endl;
}

void DummyDB::Init() {
    const utils::Properties &props = *props_;
    std::string filename = std::string(CMAKELISTS_PATH) + "/benchmarks/" + props.GetProperty("dummydb.filename", "ycsb_dataset_default.csv");
    if (!filename.ends_with(".csv")) {
        filename = filename + ".csv";  // make sure to be a csv file
    }
    std::cout << "kvs are written into " << filename << std::endl;
    file_ = new std::ofstream(filename);
    if (!file_->is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        exit(1);
    }
    *file_ << "opration, key, value" << std::endl;

    fieldcount_ = std::stoi(props.GetProperty(CoreWorkload::FIELD_COUNT_PROPERTY, CoreWorkload::FIELD_COUNT_DEFAULT));

    field_prefix_ = props.GetProperty(CoreWorkload::FIELD_NAME_PREFIX, CoreWorkload::FIELD_NAME_PREFIX_DEFAULT);

    ref_cnt_++;

    std::cout << "dummydb initialized" << std::endl;
    std::cout << "\tfilename: " << filename << std::endl;
}

void DummyDB::Cleanup() {
    if (--ref_cnt_ && file_) {
        return;
    }
    file_->close();
    delete file_;
    file_ = nullptr;
}

void DummyDB::SerializeRow(const std::vector<Field> &values, std::string *data) {
    for (const Field &field : values) {
        uint32_t len = field.name.size();
        data->append(reinterpret_cast<char *>(&len), sizeof(uint32_t));
        data->append(field.name.data(), field.name.size());
        len = field.value.size();
        data->append(reinterpret_cast<char *>(&len), sizeof(uint32_t));
        data->append(field.value.data(), field.value.size());
    }
}

void DummyDB::DeserializeRowFilter(std::vector<Field> *values, const std::string &data,
                                    const std::vector<std::string> &fields) {
    const char *p = data.data();
    const char *lim = p + data.size();

    std::vector<std::string>::const_iterator filter_iter = fields.begin();
    while (p != lim && filter_iter != fields.end()) {
        assert(p < lim);
        uint32_t len = *reinterpret_cast<const uint32_t *>(p);
        p += sizeof(uint32_t);
        std::string field(p, static_cast<const size_t>(len));
        p += len;
        len = *reinterpret_cast<const uint32_t *>(p);
        p += sizeof(uint32_t);
        std::string value(p, static_cast<const size_t>(len));
        p += len;
        if (*filter_iter == field) {
        values->push_back({field, value});
        filter_iter++;
        }
    }
    assert(values->size() == fields.size());
}

void DummyDB::DeserializeRow(std::vector<Field> *values, const std::string &data) {
    const char *p = data.data();
    const char *lim = p + data.size();
    while (p != lim) {
        assert(p < lim);
        uint32_t len = *reinterpret_cast<const uint32_t *>(p);
        p += sizeof(uint32_t);
        std::string field(p, static_cast<const size_t>(len));
        p += len;
        len = *reinterpret_cast<const uint32_t *>(p);
        p += sizeof(uint32_t);
        std::string value(p, static_cast<const size_t>(len));
        p += len;
        values->push_back({field, value});
    }
    assert(values->size() == fieldcount_);
}


std::string DummyDB::KeyFromCompKey(const std::string &comp_key) {
    size_t idx = comp_key.find(":");
    assert(idx != std::string::npos);
    return comp_key.substr(0, idx);
}

std::string DummyDB::FieldFromCompKey(const std::string &comp_key) {
    size_t idx = comp_key.find(":");
    assert(idx != std::string::npos);
    return comp_key.substr(idx + 1);
}

DB::Status DummyDB::Read(const std::string &table, const std::string &key,
                         const std::vector<std::string> *fields, std::vector<Field> &result) {
    db_get(key);
    return kOK;
}

DB::Status DummyDB::Scan(const std::string &table, const std::string &key, int len,
                         const std::vector<std::string> *fields, std::vector<std::vector<Field>> &result) {
    return kNotImplemented;
}

DB::Status DummyDB::Update(const std::string &table, const std::string &key, std::vector<Field> &values) {
    std::string data;
    SerializeRow(values, &data);
    db_get(key);
    db_put(key, data);
    return kOK;
}

DB::Status DummyDB::Insert(const std::string &table, const std::string &key, std::vector<Field> &values) {
    std::string data;
    SerializeRow(values, &data);
    db_put(key, data);
    return kOK;
}

DB::Status DummyDB::Delete(const std::string &table, const std::string &key) {
    db_delete(key);
    return kOK;
}

DB* NewDummyDB() {
    return new DummyDB;
}

const bool registerd = DBFactory::RegisterDB("dummydb", NewDummyDB);

}  // namespace ycsbc