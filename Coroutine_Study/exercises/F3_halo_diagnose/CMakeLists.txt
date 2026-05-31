cmake_minimum_required(VERSION 3.25)
project(F3_halo_diagnose LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /O2)
else()
    add_compile_options(-O2)
endif()

add_executable(F3_halo_diagnose main.cpp)

# 提示：HALO 诊断 flag
#  Clang : -Rpass=coroutine-elide -Rpass-analysis=coroutine-elide
#  GCC   : -fdump-tree-coro 然后查 dump 中是否还有 operator new 调用
#  MSVC  : Release build + 反汇编看 frame 分配是否还在堆上
