cmake_minimum_required(VERSION 3.25)
project(Capstone4_rpc_framework LANGUAGES CXX)

# 如本目录被独立打开（不通过顶层 add_subdirectory），跳过
if(NOT TARGET stdexec::stdexec AND NOT COROUTINE_STUDY_STANDALONE)
    message(WARNING "Capstone4_rpc_framework requires stage3 deps; configure via top-level CMakeLists.txt")
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict /Zi /bigobj)
endif()

# RPC 框架由多个 TU 组成（main.cpp / server.cpp / client.cpp + 头文件）
add_executable(Capstone4_rpc_framework
    src/main.cpp
    src/server.cpp
    src/client.cpp
)

# stage3 依赖：stdexec + asio（asio 通常作为 stage3 helper 的一部分）
if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(Capstone4_rpc_framework)
else()
    target_link_libraries(Capstone4_rpc_framework PRIVATE stdexec::stdexec)
endif()

# 工程内 include 优先（rpc/*.hpp）；同时也允许全局 include/
target_include_directories(Capstone4_rpc_framework PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
