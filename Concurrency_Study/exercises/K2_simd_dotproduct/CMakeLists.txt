cmake_minimum_required(VERSION 3.25)
project(K2_simd_dotproduct LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /EHsc)
endif()

# 让本题既能作为顶层子目录、也能被独立打开时找到 cmake/*Setup.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

add_executable(K2_simd_dotproduct main.cpp)
target_include_directories(K2_simd_dotproduct PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
# batch3 接入 xsimd（std::simd 的 MSVC 友好回退）：
include(XsimdSetup)
target_link_libraries(K2_simd_dotproduct PRIVATE xsimd)
