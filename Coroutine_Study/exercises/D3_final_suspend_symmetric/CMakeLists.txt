cmake_minimum_required(VERSION 3.25)
project(D3_final_suspend_symmetric LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict)
endif()

# D 模块"从零写"——不依赖 include/coroutine_study/lazy_task.hpp
add_executable(D3_final_suspend_symmetric main.cpp)
