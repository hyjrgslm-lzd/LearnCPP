cmake_minimum_required(VERSION 3.25)
project(G2_when_all_impl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8)
endif()

add_executable(G2_when_all_impl main.cpp)
