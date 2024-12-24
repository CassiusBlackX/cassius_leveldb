#include "ecmeta.h"

ECMeta::ECMeta(string path) {
    _path = path;

    struct stat sb;
    if (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        printf("metadir exists\n");
        // initialize ECMeta
        initialize();
    } else {
        mkdir(path.c_str(), 0700);
    }
}

ECMeta::~ECMeta() { }

void ECMeta::insertMeta(string stripename, ECStripe* ecstripe) {
    _ecLock.lock();
    _ecStripeMap.insert(make_pair(stripename, ecstripe));
    string filepath = _path + "/" + stripename;
    ofstream s = ofstream(filepath);

    string metastr = ecstripe->genMetaString();
    s.write(metastr.c_str(), metastr.length());
    s.close();
    _ecLock.unlock();
}

void ECMeta::initialize() {
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(_path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent -> d_name, ".") == 0 || 
                    strcmp(ent -> d_name, "..") == 0)
                continue;
            string stripename(ent->d_name);
            printf("stripename: %s\n", stripename.c_str());
            ECStripe* ecstripe = new ECStripe(stripename);
            ecstripe->setStripeMeta("ecarray/ecmeta/"+stripename);

            _ecStripeMap.insert(make_pair(stripename, ecstripe));
        }
    }
}

ECStripe* ECMeta::getECStripe(string stripename) {
    return _ecStripeMap[stripename];
}

unordered_map<string, ECStripe*> ECMeta::get_ecStripeMap(){
    return _ecStripeMap;
};