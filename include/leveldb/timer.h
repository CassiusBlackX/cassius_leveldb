#ifndef TIMER_H
#define TIMER_H

#include <iostream>
#include <map>
#include <string>
#include <sys/time.h>

namespace leveldb {
extern int printFlag;
// get time
// function with timer 单位是ms
inline long long getCurrentTime() {
  struct timeval tp;
  struct timezone tzp;
  int i;
  i = gettimeofday(&tp, &tzp);
  long long currentTime = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  return currentTime;
}

// 静态成员变量在类的外部定义
inline std::map<std::string, long long> total_times;
inline std::map<std::string, long long> variableMap;

inline void setStartTimer(const std::string& function_name) {
  //variableMap[function_name + "_start_time"] = getCurrentTime();s
}

inline void setEndTimer(const std::string& function_name) {
  //total_times[function_name] += getCurrentTime() - variableMap[function_name + "_start_time"];
}


// class Timer {
//  public:
//   // 构造函数中初始化静态成员变量
//   Timer(const std::string& function_name) : function_name_(function_name) {
//     // start_time = std::chrono::high_resolution_clock::now();
//   }

//   // 析构函数中计算时间差并更新累积时间
//   ~Timer() {
//     // auto end_time = std::chrono::high_resolution_clock::now();
//     // auto duration =
//     // std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
//     // start_time);

//     // 更新累积时间
//     // total_times[function_name_] += duration.count();
//   }

//   // 获取累积时间
//   static void printTotalTimes() {
//     for (const auto& entry : total_times) {
//       std::cout << "Total time spent in " << entry.first
//                 << "(): " << entry.second << " milliseconds." << std::endl;
//     }
//   }

//   // map清空
//   static void clearMap() { total_times.clear(); }

//  private:
//   // 静态成员变量在类的内部声明
//   static std::map<std::string, long long> total_times;
//   std::string function_name_;
//   std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
// };
// End WL
}  // namespace leveldb

#endif  // TIMER_H
