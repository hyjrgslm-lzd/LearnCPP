cmake_minimum_required(VERSION 3.28)
project(Capstone1_async_crawler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict)
endif()

add_executable(Capstone1_async_crawler main.cpp)
target_include_directories(Capstone1_async_crawler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
