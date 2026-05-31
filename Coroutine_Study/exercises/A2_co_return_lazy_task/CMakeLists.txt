cmake_minimum_required(VERSION 3.28)
project(A2_co_return_lazy_task LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /Zc:preprocessor)
endif()

add_executable(A2_co_return_lazy_task main.cpp)
target_include_directories(A2_co_return_lazy_task PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
