#ifndef LEVELDB_ECSTRIPE_H                                                                                                                                                                       
#define LEVELDB_ECSTRIPE_H                                                                                                                                                                       
#define UNUSED(x) (void)(x)                                                                                                                                                                               
        
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>

using namespace std;

class ECStripe {
    private:
        string _stripe_name;
        int _eck;
        int _ecm;
        vector<string> _filename;
        vector<long long> _filesize;

    public:
        ECStripe(string stripename);
        ~ECStripe();
        void setStripeMeta(int eck, int ecm, vector<string> dataname, vector<long long> datasize, vector<string> parityname, vector<long long> paritysize);
        void setStripeMeta(string stripepath);
        string genMetaString();
        int getEck();
        int getEcm();
        string get_stripe_name();
        vector<string> getFileName();
        vector<long long> getFileSize();

};

#endif  // LEVELDB_ECSTRIPE_H 