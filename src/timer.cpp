#include "zal_utils.h"

namespace zal_utils {
FunctionTimer::FunctionTimer(const std::string& function_name) : function_name_(function_name){
    start_time = std::chrono::high_resolution_clock::now();
}
FunctionTimer::FunctionTimer(const FunctionTimer* parent, const std::string& process_name){
    start_time = std::chrono::high_resolution_clock::now();
    function_name_ = parent->function_name_ + "::" + process_name;

}


FunctionTimer::~FunctionTimer() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    total_time[function_name_] = total_time.contains(function_name_) ? duration : std::max((long long)duration, (long long)total_time[function_name_]);  // BUG 为什么这里必须要我强制类型转换？
}

void FunctionTimer::printTotalTimes() {
    std::cout << total_time.size() << std::endl;
    for (const auto& entry : total_time) {
        std::cout << "Total time spent in " << entry.first << "(): " << entry.second << " milliseconds." << std::endl;
    }
}

void FunctionTimer::clearMap() {
    total_time.clear();
}

std::map<std::string, long long> FunctionTimer::total_time;
}
