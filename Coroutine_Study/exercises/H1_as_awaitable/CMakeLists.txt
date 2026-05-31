cmake_minimum_required(VERSION 3.25)
project(H1_as_awaitable LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8)
endif()

add_executable(H1_as_awaitable main.cpp)
target_include_directories(H1_as_awaitable PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Agent 1 提供的 helper：连接 stdexec 等 stage3 依赖
if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(H1_as_awaitable)
else()
    target_link_libraries(H1_as_awaitable PRIVATE stdexec::stdexec)
endif()
