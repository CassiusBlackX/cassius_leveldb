#include "replicalog.h"

using namespace leveldb;

ReplicaLog::ReplicaLog() {
    // 构造函数的实现
}

ReplicaLog::ReplicaLog(int replicaNum) {
    _replicaNum = replicaNum;
}

void ReplicaLog::setReplicaMeta(int replicaNum,std::vector<std::string> replicapath) {
    _replicaNum = replicaNum;
    for(int i=0; i<replicaNum; i++) {
        _log_replica_paths.push_back(replicapath[i]);
    }
}

Ecpath::Ecpath() {
    // 构造函数的实现
}

Ecpath::Ecpath(int pathNum) {
    _pathNum = pathNum;
}

void Ecpath::setEcpathMeta(int pathNum,std::vector<std::string> paths) {
    _pathNum = pathNum;
    for(int i=0; i<pathNum; i++) {
        _paths.push_back(paths[i]);
    }
}