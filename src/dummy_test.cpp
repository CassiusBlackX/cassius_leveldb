#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <string>
#include <sstream>
#include <ctime>
#include <iterator>
#include <chrono>
#include <map>

// #include <leveldb/db.h>

#include "zal_utils.h"

int main() {
    std::cout << "hello" << std::endl;
    {
    zal_utils::FunctionTimer* timer = new zal_utils::FunctionTimer("main");
     // 模拟一些操作
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 创建子类实例
    auto child_timer = timer->createFunctionTimer("Part1");

    // 模拟一些操作
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    delete timer;
    }
    zal_utils::FunctionTimer::printTotalTimes();

    return 0;
}