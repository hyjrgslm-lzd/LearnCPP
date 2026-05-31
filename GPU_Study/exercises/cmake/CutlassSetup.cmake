# CutlassSetup.cmake — FetchContent 引入 CUTLASS 3.x（header-only 部分）
include_guard(GLOBAL)

option(GPU_STUDY_FETCH_CUTLASS "通过 FetchContent 下载 CUTLASS" ON)
set(GPU_STUDY_CUTLASS_TAG "main" CACHE STRING "CUTLASS git tag 或 commit（建议锁定到 3.x release 标签）")

if(NOT GPU_STUDY_FETCH_CUTLASS)
    return()
endif()

if(TARGET cutlass)
    return()
endif()

include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/third_party" CACHE PATH "" FORCE)

# 关闭 CUTLASS 自带的重型 target（tests/profiler），只要 header
set(CUTLASS_ENABLE_TESTS        OFF CACHE BOOL "" FORCE)
set(CUTLASS_ENABLE_PROFILER     OFF CACHE BOOL "" FORCE)
set(CUTLASS_ENABLE_EXAMPLES     OFF CACHE BOOL "" FORCE)
set(CUTLASS_ENABLE_CUBLAS       OFF CACHE BOOL "" FORCE)
set(CUTLASS_ENABLE_CUDNN        OFF CACHE BOOL "" FORCE)
set(CUTLASS_ENABLE_TOOLS        OFF CACHE BOOL "" FORCE)
set(CUTLASS_ENABLE_LIBRARY      OFF CACHE BOOL "" FORCE)

FetchContent_Declare(cutlass
    GIT_REPOSITORY https://github.com/NVIDIA/cutlass.git
    GIT_TAG        ${GPU_STUDY_CUTLASS_TAG}
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE)

FetchContent_GetProperties(cutlass)
if(NOT cutlass_POPULATED)
    message(STATUS "Fetching CUTLASS (${GPU_STUDY_CUTLASS_TAG})...")
    FetchContent_Populate(cutlass)
endif()

# Header-only interface：只暴露 include 目录，跳过 CUTLASS 自带 CMake（避免其 arch 探测与 /W 级设置）
add_library(cutlass_headers INTERFACE)
target_include_directories(cutlass_headers SYSTEM INTERFACE
    ${cutlass_SOURCE_DIR}/include
    ${cutlass_SOURCE_DIR}/tools/util/include)
add_library(cutlass::headers ALIAS cutlass_headers)
