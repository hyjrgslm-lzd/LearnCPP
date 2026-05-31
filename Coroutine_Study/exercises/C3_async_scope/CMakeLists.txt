cmake_minimum_required(VERSION 3.28)
project(C3_async_scope LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict)
endif()

add_executable(C3_async_scope main.cpp)
target_include_directories(C3_async_scope PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
