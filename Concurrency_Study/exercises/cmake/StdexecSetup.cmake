# exercises/cmake/StdexecSetup.cmake
#
# 统一的 stdexec 引入逻辑（仅模块 M 的结构化并发桥接题需要，默认关闭）。
#
# 重要：stdexec 是 **header-only** 库。它自带的 CMakeLists 会经 FetchContent
# 拖入重型的 rapids-cmake / rapids-cpm 构建依赖（首次配置慢、强依赖网络、且在
# 部分环境会 bootstrap 失败）。我们**只需要它的头文件**，因此这里：
#   - 下载源码但**不处理它的 CMakeLists**（SOURCE_SUBDIR 指向不存在的子目录）；
#   - 自建一个 INTERFACE 目标 stdexec::stdexec，只暴露 include 目录；
#   - 顺带为 MSVC 加 /Zc:preprocessor（stdexec 头文件在 MSVC 下硬性要求）。
#
# 注意：stdexec 是 P2300 (std::execution) 的参考实现。本套对它仅作“桥接引用”，
# 深入内容见 Execution_Study\。

if(TARGET stdexec::stdexec)
    return()
endif()

# 创建 INTERFACE 目标的辅助函数
function(_cs_make_stdexec_target include_dir)
    add_library(cs_stdexec_headers INTERFACE)
    target_include_directories(cs_stdexec_headers INTERFACE "${include_dir}")
    target_compile_features(cs_stdexec_headers INTERFACE cxx_std_20)
    target_compile_options(cs_stdexec_headers INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)
    add_library(stdexec::stdexec ALIAS cs_stdexec_headers)
endfunction()

# 优先用本地预克隆（离线）：third_party/stdexec
get_filename_component(_cs_tp_se "${CMAKE_CURRENT_LIST_DIR}/../third_party/stdexec" ABSOLUTE)
if(EXISTS "${_cs_tp_se}/include/stdexec/execution.hpp")
    message(STATUS "[StdexecSetup] 使用本地 third_party/stdexec 的头文件（离线，绕过 rapids-cmake）。")
    _cs_make_stdexec_target("${_cs_tp_se}/include")
    return()
endif()

message(STATUS "[StdexecSetup] 下载 stdexec 源码（仅取头文件，绕过 rapids-cmake）...")

include(FetchContent)

# SOURCE_SUBDIR 指向一个不存在的目录 -> MakeAvailable 会 populate 源码，
# 但不会 add_subdirectory 它的 CMakeLists（从而不触发 rapids-cmake）。
FetchContent_Declare(
    stdexec
    GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
    SOURCE_SUBDIR  _cs_headers_only_no_cmake_
)
FetchContent_MakeAvailable(stdexec)

_cs_make_stdexec_target("${stdexec_SOURCE_DIR}/include")

message(STATUS "[StdexecSetup] stdexec headers ready (include: ${stdexec_SOURCE_DIR}/include).")
