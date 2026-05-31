cmake_minimum_required(VERSION 3.28)
project(C1_stop_token_cancel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict)
endif()

add_executable(C1_stop_token_cancel main.cpp)
target_include_directories(C1_stop_token_cancel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
