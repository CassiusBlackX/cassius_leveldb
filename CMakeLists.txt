cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(test_leveldb)

# set(${CMAKE_TOOLCHAIN_FILE} "D:/Application/vcpkg/scripts/buildsystems/vcpkg.cmake")

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(LEVELDB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/")

# 包含leveldb的头文件
include_directories(${LEVELDB_ROOT_DIR})
include_directories(${LEVELDB_ROOT_DIR}/include)
include_directories(${LEVELDB_ROOT_DIR}/port/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

# leveldb source files
set(LEVELDB_SOURCES 
    ${LEVELDB_ROOT_DIR}/db/builder.cc
    ${LEVELDB_ROOT_DIR}/db/c.cc
    ${LEVELDB_ROOT_DIR}/db/dbformat.cc
    ${LEVELDB_ROOT_DIR}/db/db_impl.cc
    ${LEVELDB_ROOT_DIR}/db/db_iter.cc
    ${LEVELDB_ROOT_DIR}/db/dumpfile.cc
    ${LEVELDB_ROOT_DIR}/db/filename.cc
    ${LEVELDB_ROOT_DIR}/db/log_reader.cc
    ${LEVELDB_ROOT_DIR}/db/log_writer.cc
    ${LEVELDB_ROOT_DIR}/db/memtable.cc
    ${LEVELDB_ROOT_DIR}/db/repair.cc
    ${LEVELDB_ROOT_DIR}/db/table_cache.cc
    ${LEVELDB_ROOT_DIR}/db/version_edit.cc
    ${LEVELDB_ROOT_DIR}/db/version_set.cc
    ${LEVELDB_ROOT_DIR}/db/write_batch.cc
    ${LEVELDB_ROOT_DIR}/table/block_builder.cc
    ${LEVELDB_ROOT_DIR}/table/block.cc
    ${LEVELDB_ROOT_DIR}/table/filter_block.cc
    ${LEVELDB_ROOT_DIR}/table/format.cc
    ${LEVELDB_ROOT_DIR}/table/iterator.cc
    ${LEVELDB_ROOT_DIR}/table/merger.cc
    ${LEVELDB_ROOT_DIR}/table/table_builder.cc
    ${LEVELDB_ROOT_DIR}/table/table.cc
    ${LEVELDB_ROOT_DIR}/table/two_level_iterator.cc
    ${LEVELDB_ROOT_DIR}/util/arena.cc
    ${LEVELDB_ROOT_DIR}/util/bloom.cc
    ${LEVELDB_ROOT_DIR}/util/cache.cc
    ${LEVELDB_ROOT_DIR}/util/coding.cc
    ${LEVELDB_ROOT_DIR}/util/comparator.cc
    ${LEVELDB_ROOT_DIR}/util/crc32c.cc
    ${LEVELDB_ROOT_DIR}/util/env.cc
    ${LEVELDB_ROOT_DIR}/util/filter_policy.cc
    ${LEVELDB_ROOT_DIR}/util/hash.cc
    ${LEVELDB_ROOT_DIR}/util/histogram.cc
    ${LEVELDB_ROOT_DIR}/util/logging.cc
    ${LEVELDB_ROOT_DIR}/util/options.cc
    ${LEVELDB_ROOT_DIR}/util/status.cc
    ${LEVELDB_ROOT_DIR}/helpers/memenv/memenv.cc
)

# ycsb source
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/ycsb/core YCSB_SOURCES)
list(FILTER YCSB_SOURCES EXCLUDE REGEX ".*ycsbc.cc$") # exclude ycsbc.cc, write our own main function
list(APPEND YCSB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ycsb/leveldbDB.cpp)
message(STATUS "YCSB_SOURCES: ${YCSB_SOURCES}")

# dummydb for ycsb
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/ycsb/core DUMMY_YCSB)
list(FILTER DUMMY_YCSB EXCLUDE REGEX ".*ycsbc.cc$") # exclude ycsbc.cc, write our own main function
list(APPEND DUMMY_YCSB ${CMAKE_CURRENT_SOURCE_DIR}/ycsb/dummyDB.cpp)
message(STATUS "DUMMY_YCSB: ${DUMMY_YCSB}")

# check OS
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    message(STATUS "Configuring for Windows")
    add_definitions(-DLEVELDB_PLATFORM_WINDOWS)
    list(APPEND LEVELDB_SOURCES ${LEVELDB_ROOT_DIR}/util/env_windows.cc)
else()  # other Unix type OS
    message(STATUS "Configuring for Linux")
    add_definitions(-DLEVELDB_PLATFORM_POSIX)
    list(APPEND LEVELDB_SOURCES ${LEVELDB_ROOT_DIR}/util/env_posix.cc)
endif()


# utils files
set(UTILS_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gen_kv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/path_string.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/timer.cpp
)


# generate executable files
# trace_kv
add_executable(ycsb_compaction_trace ${CMAKE_CURRENT_SOURCE_DIR}/src/ycsb_compaction_trace.cpp ${UTILS_FILES} ${LEVELDB_SOURCES} ${YCSB_SOURCES})
target_include_directories(ycsb_compaction_trace PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ycsb)
target_link_libraries(ycsb_compaction_trace pthread)
target_compile_definitions(ycsb_compaction_trace PRIVATE 
    CMAKELISTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}"  # pass the path of CMakeLists.txt to the compiler
    # ZAL_DEBUG  # if `ZAL_DEBUG` is defined, then lines zal changed will be compiled, or else it will be original leveldb
    # PRINT_LEVEL  # if `PRINT_LEVEL` is defined, then a lot of log infos will be compiled
    # LOG_COMPACTION  # if `LOG_COMPACTION` is defined, then we will log when compaction happens
    LOG_SST
)

# benchmark_zal
add_executable(benchmark_zal ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark_zal.cpp ${UTILS_FILES} ${LEVELDB_SOURCES})
target_link_libraries(benchmark_zal pthread
    isal
)
target_compile_definitions(benchmark_zal PRIVATE 
    CMAKELISTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}"  # pass the path of
    ZAL_DEBUG  # if `ZAL_DEBUG` is defined, then lines zal changed will be compiled, or else it will be original leveldb
    ZAL_TIMER # if `ZAL_TIMER` is defined, then we will have `FunctionTimer` counting the time
)

# dummy_test
add_executable(dummy_test ${CMAKE_CURRENT_SOURCE_DIR}/src/dummy_test.cpp 
    ${UTILS_FILES}
    ${LEVELDB_SOURCES}
)
target_link_libraries(dummy_test pthread)

# log_sst
add_executable(log_sst ${CMAKE_CURRENT_SOURCE_DIR}/src/log_sst.cpp ${UTILS_FILES} ${LEVELDB_SOURCES})
target_link_libraries(log_sst pthread)
target_include_directories(log_sst PRIVATE ${LEVELDB_ROOT_DIR}/db/)
target_compile_definitions(log_sst PRIVATE 
    CMAKELISTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}"  # pass the path of CMakeLists.txt to the compiler
    LOG_SST
    LOG_COMPACTION
    PRINT_LEVEL
)


# ex_kv_io
add_executable(ex_kv_io ${CMAKE_CURRENT_SOURCE_DIR}/src/ex_kv_io.cpp ${UTILS_FILES} ${LEVELDB_SOURCES} ${YCSB_SOURCES})
target_include_directories(ex_kv_io PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ycsb)
target_compile_definitions(ex_kv_io PRIVATE 
    CMAKELISTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}"  # pass the path of CMakeLists.txt to the compiler
    # LOG_SST
    # LOG_COMPACTION
    # PRINT_LEVEL
)
target_link_libraries(ex_kv_io pthread)

# benchmark_ycsb
add_executable(benchmark_ycsb ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark_ycsb.cpp ${YCSB_SOURCES} ${LEVELDB_SOURCES} ${UTILS_FILES})
target_include_directories(benchmark_ycsb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ycsb)
target_compile_definitions(benchmark_ycsb PRIVATE 
    CMAKELISTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}"  # pass the path of
    BENCHMARK_YCSB
    BAN_COMPACTION
)
target_link_libraries(benchmark_ycsb pthread)

# basic_ycsb_demo
add_executable(basic_ycsb_demo ${CMAKE_CURRENT_SOURCE_DIR}/src/basic_ycsb_demo.cpp ${YCSB_SOURCES} ${LEVELDB_SOURCES} ${UTILS_FILES})
target_include_directories(basic_ycsb_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ycsb)
target_compile_definitions(basic_ycsb_demo PRIVATE 
    CMAKELISTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}"  # pass the path of
)
target_link_libraries(basic_ycsb_demo pthread)

# ycsb_dataset
add_executable(ycsb_dataset ${CMAKE_CURRENT_SOURCE_DIR}/src/ycsb_dataset.cpp ${DUMMY_YCSB} ${LEVELDB_SOURCES} ${UTILS_FILES})
target_include_directories(ycsb_dataset PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ycsb)
target_compile_definitions(ycsb_dataset PRIVATE 
    CMAKELISTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}"  # pass the path of cmakelist
)
target_link_libraries(ycsb_dataset pthread)

# demo_