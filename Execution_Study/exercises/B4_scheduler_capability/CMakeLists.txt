cmake_minimum_required(VERSION 3.25)
project(B4_scheduler_capability LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8)
endif()

# 引入 stdexec —— 无论作为子项目还是独立打开都能工作
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StdexecSetup)

add_executable(B4_scheduler_capability main.cpp)
target_link_libraries(B4_scheduler_capability PRIVATE stdexec::stdexec)
