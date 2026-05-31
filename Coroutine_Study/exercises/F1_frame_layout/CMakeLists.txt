cmake_minimum_required(VERSION 3.25)
project(F1_frame_layout LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8)
endif()

add_executable(F1_frame_layout main.cpp)
# 提示：本题重点是用编译器 flag dump 协程帧布局
# GCC :  add -fdump-tree-coro          → 生成 .coro dump 文件
# MSVC:  add /d1reportSingleClassLayout<frame_class_name>
# Clang: add -Xclang -ast-dump 或 -emit-llvm
