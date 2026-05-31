cmake_minimum_required(VERSION 3.28)
project(A3_co_await_future LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /Zc:preprocessor)
endif()

add_executable(A3_co_await_future main.cpp)
target_include_directories(A3_co_await_future PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
find_package(Threads REQUIRED)
target_link_libraries(A3_co_await_future PRIVATE Threads::Threads)
