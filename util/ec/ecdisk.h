#ifndef LEVELDB_ECDisk_H                                                                                                                                                                       
#define LEVELDB_ECDisk_H                                                                                                                                                                       
#define UNUSED(x) (void)(x)                                                                                                                                                                               
        
#include <string>
#include <vector>

#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

using namespace std;

class ECDisk {
    private:
        string _path;
        int _idx;
        vector<string> _sst_files;
        int _ec_sst_idx;

    public:
        ECDisk(string path, int idx);
        ~ECDisk();
        string getFileForEC();
        vector<string> get_sst_files();


};
#endif  // LEVELDB_ECDisk_H