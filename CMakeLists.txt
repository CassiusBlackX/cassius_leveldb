cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(test_leveldb)

# set(${CMAKE_TOOLCHAIN_FILE} "D:/Application/vcpkg/scripts/buildsystems/vcpkg.cmake")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(LEVELDB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# 包含leveldb的头文件
include_directories(${LEVELDB_ROOT_DIR})
include_directories(${LEVELDB_ROOT_DIR}/include)
include_directories(${LEVELDB_ROOT_DIR}/port/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

# # Find Boost package
# find_package(Boost REQUIRED COMPONENTS system)
# # Print information about Boost package
# message(STATUS "Boost version: ${Boost_VERSION}")
# message(STATUS "Boost include directories: ${Boost_INCLUDE_DIRS}")
# message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

# # find openssl package
# find_package(OpenSSL REQUIRED)
# message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
# message(STATUS "OpenSSL include directories: ${OPENSSL_INCLUDE_DIR}")
# message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

# leveldb的源文件
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
add_executable(trace_kv ${CMAKE_CURRENT_SOURCE_DIR}/src/trace_kv.cpp ${UTILS_FILES} ${LEVELDB_SOURCES})
target_link_libraries(trace_kv pthread)
target_compile_definitions(trace_kv PRIVATE 
    CMAKELISTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}"  # pass the path of CMakeLists.txt to the compiler
    ZAL_DEBUG  # if `ZAL_DEBUG` is defined, then lines zal changed will be compiled, or else it will be original leveldb
    PRINT_LEVEL  # if `PRINT_LEVEL` is defined, then a lot of log infos will be compiled
    TRACE_KV  # if `TRACE_KV` is defined, then we will trace the kvs
    LOG_COMPACTION  # if `LOG_COMPACTION` is defined, then we will log when compaction happens
)

# check_dbs_behavior
add_executable(check_dbs_behavior ${CMAKE_CURRENT_SOURCE_DIR}/src/check_dbs_behavior.cpp ${UTILS_FILES} ${LEVELDB_SOURCES})
target_link_libraries(check_dbs_behavior pthread)
target_compile_definitions(check_dbs_behavior PRIVATE 
    CMAKELISTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}"  # pass the path of CMakeLists.txt to the compiler
    # ZAL_DEBUG  # if `ZAL_DEBUG` is defined, then lines zal changed will be compiled, or else it will be original leveldb
    # PRINT_LEVEL  # if `PRINT_LEVEL` is defined, then a lot of log infos will be compiled
    # TRACE_KV  # if `TRACE_KV` is defined, then we will trace the kvs
    # LOG_COMPACTION  # if `LOG_COMPACTION` is defined, then we will log when compaction happens
)

# benchmark_zal
# add_executable(benchmark_zal ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark_zal.cpp ${UTILS_FILES} ${LEVELDB_SOURCES})
# target_link_libraries(benchmark_zal pthread)
# target_compile_definitions(benchmark_zal PRIVATE 
#     CMAKELISTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}"  # pass the path of
#     ZAL_DEBUG  # if `ZAL_DEBUG` is defined, then lines zal changed will be compiled, or else it will be original leveldb
#     INTER_WRITE  # if `INTER_WRITE` is defined, then we will write a couple of ldb files in one time
# )

# dummy_test
add_executable(dummy_test ${CMAKE_CURRENT_SOURCE_DIR}/src/dummy_test.cpp 
    ${UTILS_FILES}
    # ${LEVELDB_SOURCES}
)
target_link_libraries(dummy_test pthread)
# target_compile_definitions()