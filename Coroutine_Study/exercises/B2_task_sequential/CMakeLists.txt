cmake_minimum_required(VERSION 3.28)
project(B2_task_sequential LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /Zc:preprocessor)
endif()

add_executable(B2_task_sequential main.cpp)
target_include_directories(B2_task_sequential PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
