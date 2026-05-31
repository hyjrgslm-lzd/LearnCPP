cmake_minimum_required(VERSION 3.28)
project(B1_recursive_generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /Zc:preprocessor)
endif()

add_executable(B1_recursive_generator main.cpp)
target_include_directories(B1_recursive_generator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
