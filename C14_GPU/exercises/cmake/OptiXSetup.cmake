# OptiXSetup.cmake — 统一 OptiX SDK 发现
#
# 搜索优先级（从高到低）：
#   1. -DOptiX_INSTALL_DIR=<path>      （CMake 缓存变量，最显式）
#   2. $ENV{OPTIX_SDK_ROOT}            （K* 旧用名，向后兼容）
#   3. $ENV{OPTIX_PATH}                （CAPSTONE2_C 旧用名，向后兼容）
#   4. Windows 默认安装位置             （C:/ProgramData/NVIDIA Corporation/OptiX SDK 8*）
#   5. Linux 常见路径                   （/usr/local/optix、/opt/optix、$ENV{HOME}/optix）
#
# 对外暴露：
#   - 缓存变量 OPTIX_INCLUDE_DIR、OPTIX_ROOT
#   - INTERFACE target  optix::headers
#   - 函数  gpu_study_optix_unavailable_stub(<target_name> <src>)
#
# 用法（练习 CMakeLists.txt 内）：
#   if(NOT OPTIX_INCLUDE_DIR)
#       gpu_study_optix_unavailable_stub(my_target host.cpp)
#       return()
#   endif()
#   add_executable(my_target host.cpp)
#   target_link_libraries(my_target PRIVATE optix::headers gpu_study_common)

include_guard(GLOBAL)

option(GPU_STUDY_NO_OPTIX "强制跳过 OptiX（用于无 OptiX 环境的 CI）" OFF)

set(_optix_search_paths "")

if(DEFINED OptiX_INSTALL_DIR AND OptiX_INSTALL_DIR)
    list(APPEND _optix_search_paths "${OptiX_INSTALL_DIR}")
    message(STATUS "[OptiX] hint -DOptiX_INSTALL_DIR=${OptiX_INSTALL_DIR}")
endif()

if(DEFINED ENV{OPTIX_SDK_ROOT} AND NOT "$ENV{OPTIX_SDK_ROOT}" STREQUAL "")
    list(APPEND _optix_search_paths "$ENV{OPTIX_SDK_ROOT}")
    message(STATUS "[OptiX] hint \$ENV{OPTIX_SDK_ROOT}=$ENV{OPTIX_SDK_ROOT}")
endif()

if(DEFINED ENV{OPTIX_PATH} AND NOT "$ENV{OPTIX_PATH}" STREQUAL "")
    list(APPEND _optix_search_paths "$ENV{OPTIX_PATH}")
    message(STATUS "[OptiX] hint \$ENV{OPTIX_PATH}=$ENV{OPTIX_PATH}")
endif()

if(WIN32)
    file(GLOB _optix_win_dirs
        "C:/ProgramData/NVIDIA Corporation/OptiX SDK 8*"
        "C:/ProgramData/NVIDIA Corporation/NVIDIA OptiX SDK 8*"
        "C:/ProgramData/NVIDIA Corporation/OptiX SDK 9*")
    list(APPEND _optix_search_paths ${_optix_win_dirs})
endif()

if(UNIX)
    list(APPEND _optix_search_paths
        "/usr/local/optix"
        "/opt/optix"
        "$ENV{HOME}/optix")
endif()

if(GPU_STUDY_NO_OPTIX)
    message(STATUS "[OptiX] GPU_STUDY_NO_OPTIX=ON — skip discovery")
    set(OPTIX_INCLUDE_DIR "" CACHE PATH "" FORCE)
else()
    find_path(OPTIX_INCLUDE_DIR
        NAMES optix.h
        PATHS ${_optix_search_paths}
        PATH_SUFFIXES include
        NO_DEFAULT_PATH)
endif()

if(OPTIX_INCLUDE_DIR)
    get_filename_component(OPTIX_ROOT "${OPTIX_INCLUDE_DIR}/.." ABSOLUTE)
    set(OPTIX_ROOT "${OPTIX_ROOT}" CACHE PATH "OptiX SDK root directory" FORCE)
    message(STATUS "[OptiX] include = ${OPTIX_INCLUDE_DIR}")
    message(STATUS "[OptiX] root    = ${OPTIX_ROOT}")

    if(NOT TARGET optix::headers)
        add_library(optix_headers INTERFACE)
        target_include_directories(optix_headers SYSTEM INTERFACE "${OPTIX_INCLUDE_DIR}")
        add_library(optix::headers ALIAS optix_headers)
    endif()
else()
    message(STATUS "[OptiX] not found — OptiX 练习将以 stub 模式构建")
    message(STATUS "[OptiX]   提示：cmake -DOptiX_INSTALL_DIR=<path>  或")
    message(STATUS "[OptiX]         export OPTIX_SDK_ROOT=<path>      或")
    message(STATUS "[OptiX]         export OPTIX_PATH=<path>")
    message(STATUS "[OptiX]   下载：https://developer.nvidia.com/designworks/optix/download")
endif()

# ─────────────────────────────────────────────────────────────────────────────
# stub 辅助函数：OptiX 不可用时为练习生成占位 target
# ─────────────────────────────────────────────────────────────────────────────
function(gpu_study_optix_unavailable_stub target_name source_file)
    message(STATUS "[OptiX] ${target_name}: stub 模式（缺 OptiX SDK）")
    add_executable(${target_name} "${source_file}")
    target_link_libraries(${target_name} PRIVATE gpu_study_common)
    target_compile_definitions(${target_name} PRIVATE GPU_STUDY_NO_OPTIX=1)
endfunction()
