cmake_minimum_required(VERSION 3.25)
project(Capstone4_rpc_framework LANGUAGES CXX)

include(CTest)

# 如本目录被独立打开（不通过顶层 add_subdirectory），跳过
if(NOT TARGET asio::asio AND NOT COROUTINE_STUDY_STANDALONE)
    message(WARNING "Capstone4_rpc_framework requires asio; configure via top-level CMakeLists.txt")
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

if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(Capstone4_rpc_framework)
else()
    target_link_libraries(Capstone4_rpc_framework PRIVATE asio::asio)
endif()

# 工程内 include 优先（rpc/*.hpp）；同时也允许全局 include/
target_include_directories(Capstone4_rpc_framework PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)

if(COROUTINE_STUDY_BUILD_REFERENCE)
    add_executable(Capstone4_rpc_framework_reference
        reference/tests/rpc_reference_test.cpp
    )
    target_include_directories(Capstone4_rpc_framework_reference PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/reference/include
    )
    target_link_libraries(Capstone4_rpc_framework_reference PRIVATE asio::asio)
    add_test(NAME Capstone4_rpc_framework_reference COMMAND Capstone4_rpc_framework_reference)
    set_tests_properties(Capstone4_rpc_framework_reference PROPERTIES
        LABELS "reference;rpc"
        TIMEOUT 60
    )
endif()
