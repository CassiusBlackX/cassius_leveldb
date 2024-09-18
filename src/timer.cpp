#include "zal_utils.h"

namespace zal_utils {
FunctionTimer::FunctionTimer(const std::string& function_name) : function_name_(function_name) {
    start_time = std::chrono::high_resolution_clock::now();
}
FunctionTimer::FunctionTimer(const FunctionTimer& parent, const std::string& process_name) {
    start_time = std::chrono::high_resolution_clock::now();
    function_name_ = parent.function_name_ + "::" + process_name;
}


FunctionTimer::~FunctionTimer() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    total_times[function_name_] += duration.count();
}

void FunctionTimer::printTotalTimes() {
    std::cout << total_times.size() << std::endl;
    for (const auto& entry : total_times) {
        std::cout << "Total time spent in " << entry.first << "(): " << entry.second << " milliseconds." << std::endl;
    }
}

void FunctionTimer::clearMap() {
    total_times.clear();
}

std::map<std::string, long long> FunctionTimer::total_times;
}