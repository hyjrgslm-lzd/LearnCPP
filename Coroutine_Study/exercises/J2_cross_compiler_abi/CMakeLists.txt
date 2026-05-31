cmake_minimum_required(VERSION 3.25)
project(J2_cross_compiler_abi LANGUAGES CXX)

# 如本目录被独立打开（不通过顶层 add_subdirectory），跳过
if(NOT TARGET stdexec::stdexec AND NOT COROUTINE_STUDY_STANDALONE)
    message(WARNING "J2_cross_compiler_abi requires stage3 deps; configure via top-level CMakeLists.txt")
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(J2_cross_compiler_abi main.cpp)

# 注入编译器特定的诊断 flag —— 用于 dump 协程帧布局 / HALO 报告
if(MSVC)
    target_compile_options(J2_cross_compiler_abi PRIVATE
        /Zc:__cplusplus
        /utf-8
        /await:strict
        /Zi                                                # 调试信息
        # 提示：要 dump promise 布局，可在命令行加：
        #   /d1reportSingleClassLayoutpromise_type
        # 由于会污染所有 TU，CMake 中默认不开
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_compile_options(J2_cross_compiler_abi PRIVATE
        -Rpass=coroutine-elide
        -Rpass-missed=coroutine-elide
        # -Xclang -fdump-record-layouts                     # 可选：dump record 布局
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(J2_cross_compiler_abi PRIVATE
        -fdump-tree-coro
        -fdump-ipa-coro
    )
endif()

if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(J2_cross_compiler_abi)
else()
    target_link_libraries(J2_cross_compiler_abi PRIVATE stdexec::stdexec)
endif()

target_include_directories(J2_cross_compiler_abi PRIVATE ${CMAKE_SOURCE_DIR}/include)
