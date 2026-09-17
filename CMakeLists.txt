cmake_minimum_required(VERSION 3.15)
project(OpenShard)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(CURL QUIET)

# Use nlohmann/json header-only (will be created locally)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann/json.hpp)
  message(STATUS "Downloading nlohmann/json...")
  file(DOWNLOAD https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp 
       ${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann/json.hpp)
endif()

# Phase 0: Remote Range Retrieval
if(CURL_FOUND)
  add_executable(phase0_remote_range src/phase0/remote_range.cpp)
  target_link_libraries(phase0_remote_range PRIVATE CURL::libcurl)
  target_include_directories(phase0_remote_range PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
else()
  message(STATUS "CURL not found, skipping phase0_remote_range")
endif()

# Phase 1: Archive Reader
add_library(archive_reader src/archive/tar_reader.cpp)
target_include_directories(archive_reader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Phase 2: Index
add_library(openshard_index src/index/index.cpp)
target_include_directories(openshard_index PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Phase 3: Retrieval Engine
add_library(retrieval_engine src/retrieval/retriever.cpp)
target_include_directories(retrieval_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(retrieval_engine PUBLIC openshard_index)

# Tests
enable_testing()

if(CURL_FOUND)
  add_executable(test_phase0 tests/test_phase0.cpp)
  target_link_libraries(test_phase0 PRIVATE CURL::libcurl)
  target_include_directories(test_phase0 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  add_test(NAME Phase0Test COMMAND test_phase0)
endif()

# Phase 4: Gateway
add_library(openshard_gateway src/gateway/gateway.cpp)
target_include_directories(openshard_gateway PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(openshard_gateway PUBLIC retrieval_engine)

# Phase 5: Batch Retrieval
add_library(batch_retriever src/retrieval/batch_retriever.cpp)
target_include_directories(batch_retriever PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(batch_retriever PUBLIC retrieval_engine)

# Phase 6: Retrieval Planner
add_library(retrieval_planner src/retrieval/planner.cpp)
target_include_directories(retrieval_planner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(retrieval_planner PUBLIC openshard_index)

# Phase 7: Caching
add_library(openshard_cache src/storage/cache.cpp)
target_include_directories(openshard_cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Phase 8: Concurrency
# add_library(thread_pool src/concurrency/thread_pool.cpp)
# target_include_directories(thread_pool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Phase 9: Evaluation
add_library(benchmark_evaluator src/benchmark/evaluator.cpp)
target_include_directories(benchmark_evaluator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(benchmark_evaluator PUBLIC retrieval_engine openshard_index)

# Phase 10: Archive Reader abstraction - not used in tests
# add_library(archive_abstraction src/archive/archive_reader.cpp)
# target_include_directories(archive_abstraction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# target_link_libraries(archive_abstraction PUBLIC archive_reader)

# Tests
add_executable(test_phase123 tests/test_phase123.cpp)
target_link_libraries(test_phase123 PRIVATE archive_reader openshard_index retrieval_engine)
target_include_directories(test_phase123 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME Phase123Test COMMAND test_phase123)

add_executable(test_phases_advanced tests/test_phases_advanced.cpp)
target_link_libraries(test_phases_advanced PRIVATE 
  batch_retriever retrieval_planner openshard_cache 
  benchmark_evaluator openshard_gateway)
target_include_directories(test_phases_advanced PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME PhasesAdvancedTest COMMAND test_phases_advanced)
