#ifndef LEVELDB_ReplicaLog_H
#define LEVELDB_ReplicaLog_H
#define UNUSED(x) (void)(x)

#include <dirent.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace leveldb {
class ReplicaLog {
 private:
  int _replicaNum;
  std::vector<std::string> _log_replica_paths;

 public:
  static ReplicaLog& getNullInstance() {
    static ReplicaLog nullInstance;  // 适当初始化为空对象
    return nullInstance;
  }
  ReplicaLog();
  ReplicaLog(int replicaNum);
  //~ReplicaLog();
  ~ReplicaLog() = default;  // 使用默认的析构函数
  void setReplicaMeta(int replicaNum, std::vector<std::string> replicapath);

  // 构造函数接受 nullptr
  ReplicaLog(std::nullptr_t) {
    // 如果需要，初始化为空对象或采取其他措施
  }

  // 添加getter方法
  int getReplicaNum() const { return _replicaNum; }

  const std::vector<std::string>& getLogReplicaPaths() const {
    return _log_replica_paths;
  }
};

class Ecpath {
 private:
  int _pathNum;
  std::vector<std::string> _paths;

 public:
  static Ecpath& getNullInstance() {
    static Ecpath nullInstance;  // 适当初始化为空对象
    return nullInstance;
  }
  Ecpath();
  Ecpath(int pathNum);
  //~Ecpath();
  ~Ecpath() = default;  // 使用默认的析构函数
  void setEcpathMeta(int pathNum, std::vector<std::string> paths);

  // 构造函数接受 nullptr
  Ecpath(std::nullptr_t) {
    // 如果需要，初始化为空对象或采取其他措施
  }

  // 添加getter方法
  int getEcpathNum() const { return _pathNum; }

  const std::vector<std::string>& getEcpath() const {
    return _paths;
  }
};
}  // namespace leveldb

#endif  // LEVELDB_ReplicaLog_H
