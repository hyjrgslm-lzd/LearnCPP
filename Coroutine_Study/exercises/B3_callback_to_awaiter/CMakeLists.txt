cmake_minimum_required(VERSION 3.28)
project(B3_callback_to_awaiter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /Zc:preprocessor)
endif()

add_executable(B3_callback_to_awaiter main.cpp)
target_include_directories(B3_callback_to_awaiter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
find_package(Threads REQUIRED)
target_link_libraries(B3_callback_to_awaiter PRIVATE Threads::Threads)
