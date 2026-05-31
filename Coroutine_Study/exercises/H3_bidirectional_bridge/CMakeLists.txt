cmake_minimum_required(VERSION 3.25)
project(H3_bidirectional_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8)
endif()

add_executable(H3_bidirectional_bridge main.cpp)
target_include_directories(H3_bidirectional_bridge PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(H3_bidirectional_bridge)
else()
    target_link_libraries(H3_bidirectional_bridge PRIVATE stdexec::stdexec)
endif()
