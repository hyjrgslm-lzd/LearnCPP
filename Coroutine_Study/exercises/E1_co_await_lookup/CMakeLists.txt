cmake_minimum_required(VERSION 3.25)
project(E1_co_await_lookup LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict)
endif()

# E 模块"自定义 promise 实验"——不依赖 include/coroutine_study/lazy_task.hpp
add_executable(E1_co_await_lookup main.cpp)
