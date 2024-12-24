#include "ecdisk.h"

ECDisk::ECDisk(string path, int idx) {
    _path = path;
    _idx = idx;

    struct stat sb;
    bool exist = false;
    if (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        printf("disk %d exists\n", _idx);
        exist = true;
    } else {
        mkdir(path.c_str(), 0700);
    }

    if (exist) {
        // read disk files from path
        DIR* dir;
        struct dirent* ent;
        if ((dir = opendir(path.c_str())) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (strcmp(ent -> d_name, ".") == 0 ||
                        strcmp(ent -> d_name, "..") == 0) {
                    continue;
                }
                //printf("sst: %s\n", ent->d_name);

                string f(ent->d_name);
                _sst_files.push_back(f);
            }    
        }

        //printf("sst files: \n");
        //for (int i=0; i<(int)_sst_files.size(); i++)
        //    printf("  %s\n", _sst_files[i].c_str());
    }

    _ec_sst_idx = 0;
}

ECDisk::~ECDisk() {

}

string ECDisk::getFileForEC() {
    string toret = "";
    if (_ec_sst_idx < (int)_sst_files.size())
        toret = "ecarray/disk"+to_string(_idx)+"/"+_sst_files[_ec_sst_idx++];
    else
        toret = "null-"+ to_string(_idx);
    return toret;
}

vector<string> ECDisk::get_sst_files(){
    return _sst_files;
}