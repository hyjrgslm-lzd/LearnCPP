cmake_minimum_required(VERSION 3.25)
project(D1_promise_8_hooks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict)
endif()

# 注意：D 模块起，禁止包含 include/coroutine_study/lazy_task.hpp
# 你必须从空白处自己写出整个 lazy_task<T>。
add_executable(D1_promise_8_hooks main.cpp)
