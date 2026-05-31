cmake_minimum_required(VERSION 3.25)
project(I1_atomic_shared_ptr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /EHsc)
endif()

# 让本题既能作为顶层子目录、也能被独立打开时找到 cmake/*Setup.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

add_executable(I1_atomic_shared_ptr main.cpp)
target_include_directories(I1_atomic_shared_ptr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
