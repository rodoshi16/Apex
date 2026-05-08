cmake_minimum_required(VERSION 3.20)
project(apex VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main)

FetchContent_Declare(googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG main)

FetchContent_MakeAvailable(googletest googlebenchmark)

add_library(apex_lib INTERFACE)
target_include_directories(apex_lib INTERFACE include)

add_executable(apex src/main.cpp)
target_link_libraries(apex PRIVATE apex_lib)
target_compile_options(apex PRIVATE -Wall -Wextra -O2)

add_subdirectory(tests)
add_subdirectory(benchmarks)