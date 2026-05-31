# exercises/cmake/TbbSetup.cmake
#
# 并行 STL（std::execution::par / par_unseq）的后端引入逻辑（模块 L）。
#
# 平台差异（已核验）：
#   - MSVC：标准库内置完整并行算法支持，无需任何外部依赖、无需链接任何库。
#           因此在 Windows + VS2026 主路径下，本文件什么都不做。
#   - GCC (libstdc++)：并行算法的 par/par_unseq 后端历史上依赖 Intel TBB，
#           需要 find_package(TBB) 并链接 TBB::tbb。
#   - Clang (libc++)：并行算法支持随版本而异，部分配置同样需要 TBB。
#
# 用法：模块 L 的题目 CMakeLists.txt：
#   include(TbbSetup)
#   target_link_libraries(<target> PRIVATE ${CONCURRENCY_STUDY_PAR_LIBS})
# 在 MSVC 上 ${CONCURRENCY_STUDY_PAR_LIBS} 为空，链接行为无副作用。

set(CONCURRENCY_STUDY_PAR_LIBS "" CACHE INTERNAL "并行 STL 后端链接库")

if(MSVC)
    message(STATUS "[TbbSetup] MSVC 内置并行 STL 支持，无需 TBB。")
    return()
endif()

# 非 MSVC：尝试找到 TBB
find_package(TBB QUIET)
if(TBB_FOUND)
    set(CONCURRENCY_STUDY_PAR_LIBS "TBB::tbb" CACHE INTERNAL "并行 STL 后端链接库")
    message(STATUS "[TbbSetup] 找到 TBB，并行算法将使用 TBB 后端。")
else()
    message(WARNING
        "[TbbSetup] 未找到 TBB。在 GCC/Clang 上 std::execution::par 可能退化为串行或链接失败。\n"
        "  安装方式：apt install libtbb-dev / brew install tbb / vcpkg install tbb")
endif()
