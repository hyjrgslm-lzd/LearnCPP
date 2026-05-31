cmake_minimum_required(VERSION 3.25)
project(M2_execution_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # /Zc:preprocessor 为 stdexec 强制要求（其头文件用了符合标准的预处理器）。
    add_compile_options(/Zc:__cplusplus /utf-8 /EHsc /Zc:preprocessor)
endif()

# 让本题既能作为顶层子目录、也能被独立打开时找到 cmake/*Setup.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

add_executable(M2_execution_bridge main.cpp)
target_include_directories(M2_execution_bridge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
# batch3 接入 stdexec（std::execution 桥接）：
include(StdexecSetup)
target_link_libraries(M2_execution_bridge PRIVATE stdexec::stdexec)
