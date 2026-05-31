# exercises/cmake/XsimdSetup.cmake
#
# 统一的 xsimd 引入逻辑（模块 K：数据并行与 std::simd）。
#
# 为什么用 xsimd 作回退：
#   C++26 的 std::simd (<simd>, P1928) 截至 2026-05 主流编译器尚未实现
#   （仅 GCC 16 部分实现），MSVC 完全没有。
#   xsimd 是 header-only、跨平台（MSVC/GCC/Clang 全支持）的 SIMD 抽象库，
#   API 与 std::simd 概念高度对应（batch/batch_bool、load/store、reduce、select），
#   是 Windows + VS2026 上唯一能真正编译运行的方案。
#   文档正文以 std::simd 标准 API 为讲解目标，并给出 xsimd <-> std::simd 差异映射表。
#
# 与 StdexecSetup 相同：TARGET 守卫保证独立打开单题也能工作。

if(TARGET xsimd)
    return()
endif()

# 优先用本地预克隆（离线）：third_party/xsimd
get_filename_component(_cs_tp "${CMAKE_CURRENT_LIST_DIR}/../third_party/xsimd" ABSOLUTE)
if(EXISTS "${_cs_tp}/include/xsimd/xsimd.hpp")
    message(STATUS "[XsimdSetup] 使用本地 third_party/xsimd（离线）。")
    add_subdirectory("${_cs_tp}" "${CMAKE_BINARY_DIR}/_local_xsimd" EXCLUDE_FROM_ALL)
    return()
endif()

message(STATUS "[XsimdSetup] xsimd not found — using FetchContent to download it...")

include(FetchContent)

FetchContent_Declare(
    xsimd
    GIT_REPOSITORY https://github.com/xtensor-stack/xsimd.git
    GIT_TAG        13.2.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

# xsimd 是 header-only；关掉它自带的测试/基准
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(XSIMD_SKIP_INSTALL ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(xsimd)

message(STATUS "[XsimdSetup] xsimd ready.")
