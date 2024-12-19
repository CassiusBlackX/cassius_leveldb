#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>

#include <leveldb/db.h>

#ifndef CMAKELISTS_PATH
#define CMAKELISTS_PATH "."
#endif

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

struct YCSBDataset{
    enum Operation {
        PUT,
        GET
    };

    Operation op;
    std::string key;
    std::string value;

    YCSBDataset(const std::string& operation, const std::string& key, const std::string& value)
        : key(key), value(Base64Decode(value)) {
            if (operation == "put") {
                op = PUT;
            } else if (operation == "get") {
                op = GET;
            } else {
                throw std::invalid_argument("Invalid operation: " + operation);
            }
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
};

int main() {
    const std::string dbName = "testdb";
    leveldb::Options options;
    leveldb::DestroyDB(dbName, options);
    leveldb::DB* db;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, dbName, &db);

    const std::string dataset_path = std::string(CMAKELISTS_PATH) + "/dataset/ycsb_dataset_KV1K_2GB.csv";
    std::ifstream input(dataset_path);
    if(!input.is_open()) {
        std::cerr << "Error: could not open file " << dataset_path << std::endl;
        return 1;
    }
    std::string line;
    std::getline(input, line); // skip header

    std::string operation;
    std::string key;
    std::string value;

    std::vector<YCSBDataset> dataset;

    while (std::getline(input, line)) {
        std::stringstream ss(line);
        if (std::getline(ss, operation, ',')
            && std::getline(ss, key, ',')
            && std::getline(ss, value, ',')) {
                dataset.emplace_back(operation, key, value);
        }
    }

    for (const auto& data : dataset) {
        switch (data.op) {
            case YCSBDataset::PUT:
                status = db->Put(leveldb::WriteOptions(), data.key, data.value);
                if (!status.ok()) {
                    std::cerr << "Error: " << status.ToString() << std::endl;
                }
                break;
            case YCSBDataset::GET:
                status = db->Get(leveldb::ReadOptions(), data.key, &value);
                if (!status.ok()) {
                    std::cerr << "Error: " << status.ToString() << std::endl;
                }
                break;
            default:
                break;
        }
    }
    
    input.close();
    delete db;
    return 0;   
}