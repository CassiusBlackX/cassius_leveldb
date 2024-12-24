#ifndef LEVELDB_ECMETA_H 
#define LEVELDB_ECMETA_H
#define UNUSED(x) (void)(x)

#include <string>
#include <fstream>
#include <sys/stat.h>
#include <unordered_map>
#include <mutex>

#include <string.h>                                                                                                                                                                                       
#include <sys/stat.h>                                                                                                                                                                                     
#include <dirent.h>

using namespace std;

class ECMeta {
    private:
        string _path;
        unordered_map<string, ECStripe*> _ecStripeMap;
        mutex _ecLock;

        void initialize();

    public:
        ECMeta(string path);
        ~ECMeta();

        void insertMeta(string stripename, ECStripe* ecstripe);
        ECStripe* getECStripe(string stripename);
        unordered_map<string, ECStripe*> get_ecStripeMap();
};
#endif  // LEVELDB_ECMETA_H 