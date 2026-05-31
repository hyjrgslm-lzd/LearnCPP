cmake_minimum_required(VERSION 3.25)
project(C1_condvar_predicate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /EHsc)
endif()

# 让本题既能作为顶层子目录、也能被独立打开时找到 cmake/*Setup.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

add_executable(C1_condvar_predicate main.cpp)
target_include_directories(C1_condvar_predicate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
