cmake_minimum_required(VERSION 3.25)
project(A3_thread_exception LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /EHsc)
endif()

# 让本题既能作为顶层子目录、也能被独立打开时找到 cmake/*Setup.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

add_executable(A3_thread_exception main.cpp)
target_include_directories(A3_thread_exception PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
