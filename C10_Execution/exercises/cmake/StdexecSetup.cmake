# exercises/cmake/StdexecSetup.cmake
#
# 统一的 stdexec 引入逻辑。
# 当子目录作为独立项目打开时（VS Code 打开单个习题），
# 由子目录的 CMakeLists.txt include 此文件来拉取 stdexec。
# 当子目录作为顶层项目的子目录时（顶层已配置 stdexec），此文件什么也不做。

# 如果 stdexec::stdexec 已经存在（被顶层 CMake 提供），跳过
if(TARGET stdexec::stdexec)
    return()
endif()

# 也检查大写版本
if(TARGET STDEXEC::stdexec)
    if(NOT TARGET stdexec::stdexec)
        add_library(stdexec::stdexec ALIAS stdexec)
    endif()
    return()
endif()

message(STATUS "[StdexecSetup] stdexec not found — using FetchContent to download it...")

include(FetchContent)

FetchContent_Declare(
    stdexec
    GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

set(STDEXEC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(STDEXEC_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(STDEXEC_BUILD_DOCS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(stdexec)

# 创建小写 alias
if(NOT TARGET stdexec::stdexec)
    add_library(stdexec::stdexec ALIAS stdexec)
endif()

message(STATUS "[StdexecSetup] stdexec ready.")
