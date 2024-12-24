#include "ecstripe.h"

ECStripe::ECStripe(string stripename) {
    _stripe_name = stripename;
}

ECStripe::~ECStripe() { }

string ECStripe::get_stripe_name(){
    return _stripe_name;
}

void ECStripe::setStripeMeta(int eck, int ecm, vector<string> dataname, vector<long long> datasize, vector<string> parityname, vector<long long> paritysize) {

    _eck = eck;
    _ecm = ecm;

    for (int i=0; i<eck; i++){
        _filename.push_back(dataname[i]);
        _filesize.push_back(datasize[i]);
    }
    for (int i=0; i<ecm; i++) {
        _filename.push_back(parityname[i]);
        _filesize.push_back(paritysize[i]);
    }
}

void ECStripe::setStripeMeta(string stripepath) {
    printf("stripepath: %s\n", stripepath.c_str());
    ifstream file(stripepath);
    string line;

    // eck
    getline(file, line);
    _eck = atoi(line.c_str());
    printf("eck: %d\n", _eck);
    // ecm
    getline(file, line);
    _ecm = atoi(line.c_str());
    printf("ecm: %d\n", _ecm);
    
    for (int i=0; i<_eck+_ecm; i++) {
        getline(file, line);
        // filename
        _filename.push_back(line);
        // filesize
        getline(file, line);
        int size = atoi(line.c_str());
        _filesize.push_back(size);
    }

    for (int i=0; i<_eck+_ecm; i++) {
        printf("%s, %lld\n", _filename[i].c_str(), _filesize[i]);
    }
}

string ECStripe::genMetaString() {
    string toret = "";
    toret += to_string(_eck) + "\n";
    toret += to_string(_ecm) + "\n";
    for (int i=0; i<_eck+_ecm; i++) {    
        toret += _filename[i] + "\n" + to_string(_filesize[i]) + "\n";
    }
    return toret;
}

int ECStripe::getEck() {
    return _eck;
}

int ECStripe::getEcm() {
    return _ecm;
}

vector<string> ECStripe::getFileName() {
    return _filename;
}

vector<long long> ECStripe::getFileSize() {
    return _filesize;
}