#include <leveldb/options.h>
#include <leveldb/write_batch.h>
#include <leveldb/cache.h>
#include <leveldb/filter_policy.h>

#include "leveldbDB.h"
#include "core/core_workload.h"
#include "core/db_factory.h"
#include "utils/utils.h"

namespace {
  const std::string PROP_NAME = "leveldb.dbname";
  const std::string PROP_NAME_DEFAULT = "testdb";

  const std::string PROP_FORMAT = "leveldb.format";
  const std::string PROP_FORMAT_DEFAULT = "single";

  const std::string PROP_DESTROY = "leveldb.destroy";  // destroy the db before open
  const std::string PROP_DESTROY_DEFAULT = "false";  // we will need it to be true, but default should be false

  const std::string PROP_COMPRESSION = "leveldb.compression";
  const std::string PROP_COMPRESSION_DEFAULT = "no";

  const std::string PROP_WRITE_BUFFER_SIZE = "leveldb.write_buffer_size";
  const std::string PROP_WRITE_BUFFER_SIZE_DEFAULT = "0";

  const std::string PROP_MAX_FILE_SIZE = "leveldb.max_file_size";
  const std::string PROP_MAX_FILE_SIZE_DEFAULT = "0";

  const std::string PROP_MAX_OPEN_FILES = "leveldb.max_open_files";
  const std::string PROP_MAX_OPEN_FILES_DEFAULT = "0";

  const std::string PROP_CACHE_SIZE = "leveldb.cache_size";
  const std::string PROP_CACHE_SIZE_DEFAULT = "0";

  const std::string PROP_FILTER_BITS = "leveldb.filter_bits";
  const std::string PROP_FILTER_BITS_DEFAULT = "0";

  const std::string PROP_BLOCK_SIZE = "leveldb.block_size";
  const std::string PROP_BLOCK_SIZE_DEFAULT = "0";

  const std::string PROP_BLOCK_RESTART_INTERVAL = "leveldb.block_restart_interval";
  const std::string PROP_BLOCK_RESTART_INTERVAL_DEFAULT = "0";
}  // anonymous namespace

namespace ycsbc {
leveldb::DB *LeveldbDB::db_ = nullptr;
int LeveldbDB::ref_cnt_ = 0;
std::mutex LeveldbDB::mu_;  // for the sake of multi-threading

void LeveldbDB::Init() {
    const std::lock_guard<std::mutex> lock(mu_);

    const utils::Properties &props = *props_;
    const std::string& format = props.GetProperty(PROP_FORMAT, PROP_FORMAT_DEFAULT);
    if (format == "single") {
        format_ = kSingleEntry;
        method_read_ = &LeveldbDB::ReadSingleEntry;
        method_scan_ = &LeveldbDB::ScanSingleEntry;
        method_update_ = &LeveldbDB::UpdateSingleEntry;
        method_insert_ = &LeveldbDB::InsertSingleEntry;
        method_delete_ = &LeveldbDB::DeleteSingleEntry;
    } else if (format == "row") {
        format_ = kRowMajor;
        method_read_ = &LeveldbDB::ReadCompKeyRM;
        method_scan_ = &LeveldbDB::ScanCompKeyRM;
        method_update_ = &LeveldbDB::InsertCompKey;
        method_insert_ = &LeveldbDB::InsertCompKey;
        method_delete_ = &LeveldbDB::DeleteCompKey;
    } else if (format == "column") {
        format_ = kColumnMajor;
        method_read_ = &LeveldbDB::ReadCompKeyCM;
        method_scan_ = &LeveldbDB::ScanCompKeyCM;
        method_update_ = &LeveldbDB::InsertCompKey;
        method_insert_ = &LeveldbDB::InsertCompKey;
        method_delete_ = &LeveldbDB::DeleteCompKey;
    } else {
        throw utils::Exception("unknown format");
    }

    field_count_ = std::stoi(props.GetProperty(CoreWorkload::FIELD_COUNT_PROPERTY, CoreWorkload::FIELD_COUNT_DEFAULT));
    field_prefix_ = props.GetProperty(CoreWorkload::FIELD_NAME_PREFIX, CoreWorkload::FIELD_NAME_PREFIX_DEFAULT);

    ref_cnt_++;
    if (db_) {
        return;
    }

    const std::string& db_path = props.GetProperty(PROP_NAME, PROP_NAME_DEFAULT);
    if (db_path == "") {
        throw utils::Exception("leveldb db path cannot be empty");
    }

    leveldb::Options options;
    options.create_if_missing = true;
    GetOptions(props, options);

    leveldb::Status s;

    if (props.GetProperty(PROP_DESTROY, PROP_DESTROY_DEFAULT) == "true") {
        s = leveldb::DestroyDB(db_path, options);
        if (!s.ok()) {
            throw utils::Exception(std::string("leveldb DestroyDB: ") + s.ToString());
        }
    }
    s = leveldb::DB::Open(options, db_path, &db_);
    if (!s.ok()) {
        throw utils::Exception(std::string("leveldb Open: ") + s.ToString());
    }
}

void LeveldbDB::Cleanup() {
    const std::lock_guard<std::mutex> lock(mu_);

    if (--ref_cnt_) {
        return;
    }
    delete db_;
}

void LeveldbDB::GetOptions(const utils::Properties& props, leveldb::Options& options) {
    size_t writer_buffer_size = std::stoull(props.GetProperty(PROP_WRITE_BUFFER_SIZE, PROP_WRITE_BUFFER_SIZE_DEFAULT));
    if (writer_buffer_size) {  // as long as it is not 0, we will use it
        options.write_buffer_size = writer_buffer_size;  // Bytes
    }
    
    size_t max_file_size = std::stoull(props.GetProperty(PROP_MAX_FILE_SIZE, PROP_MAX_FILE_SIZE_DEFAULT));
    if (max_file_size) {
        options.max_file_size = max_file_size;  // Bytes
    }

    size_t cache_size = std::stoull(props.GetProperty(PROP_CACHE_SIZE, PROP_CACHE_SIZE_DEFAULT));
    if (cache_size) {
        options.block_cache = leveldb::NewLRUCache(cache_size);  // Bytes
    }

    size_t max_open_files = std::stoull(props.GetProperty(PROP_MAX_OPEN_FILES, PROP_MAX_OPEN_FILES_DEFAULT));
    if (max_open_files) {
        options.max_open_files = max_open_files;
    }

    std::string compression = props.GetProperty(PROP_COMPRESSION, PROP_COMPRESSION_DEFAULT);
    if (compression == "snappy") {
        options.compression = leveldb::kSnappyCompression;
    } else {
        options.compression = leveldb::kNoCompression;  // TODO did not take `zstd` into account
    }

    int filter_bits = std::stoi(props.GetProperty(PROP_FILTER_BITS, PROP_FILTER_BITS_DEFAULT));
    if (filter_bits > 0) {
        options.filter_policy = leveldb::NewBloomFilterPolicy(filter_bits);
    }

    size_t block_size = std::stoull(props.GetProperty(PROP_BLOCK_SIZE, PROP_BLOCK_SIZE_DEFAULT));
    if (block_size) {
        options.block_size = block_size;  // Bytes
    }

    int block_restart_interval = std::stoi(props.GetProperty(PROP_BLOCK_RESTART_INTERVAL, PROP_BLOCK_RESTART_INTERVAL_DEFAULT));
    if (block_restart_interval > 0) {
        options.block_restart_interval = block_restart_interval;
    }
}

void LeveldbDB::SerializeRow(const std::vector<Field>& values, std::string& data) {
    for (const Field& field: values) {
        size_t len = field.value.size();
        data.append(reinterpret_cast<const char*>(&len), sizeof(size_t));
        data.append(field.value.data(), field.name.size());
        len = field.value.size();
        data.append(reinterpret_cast<const char*>(&len), sizeof(size_t));
        data.append(field.value.data(), field.value.size());
    }
} 

void LeveldbDB::DeserializeRowFilter(std::vector<Field>& values, const std::string& data, const std::vector<std::string>& fields) {
    const char* p = data.data();
}

}// namespace ycsbc