#include "zal_utils.h"

namespace zal_utils {
FunctionTimer::FunctionTimer(const std::string& function_name) : function_name_(function_name) {
    start_time = std::chrono::high_resolution_clock::now();
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

std::unique_ptr<FunctionTimer> FunctionTimer::createFunctionTimer(const std::string& function_name) {
    return std::make_unique<FunctionTimer>(function_name_ + "::" + function_name);
}

void FunctionTimer::clearMap() {
    total_times.clear();
}

std::map<std::string, long long> FunctionTimer::total_times;
}