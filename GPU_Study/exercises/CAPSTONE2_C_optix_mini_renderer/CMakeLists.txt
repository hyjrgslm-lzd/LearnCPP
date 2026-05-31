cmake_minimum_required(VERSION 3.28)
project(CAPSTONE2_C_optix_mini_renderer LANGUAGES CXX CUDA)

# ============================================================
# 分支 C：OptiX mini 路径追踪渲染器（Cornell Box + Denoiser）
#
# 依赖：OptiX SDK 8.x（由根 cmake/OptiXSetup.cmake 统一发现）
# GPU：任意 RTX（sm_75+）；Hopper 不是必须条件
# 若 OptiX 不可用，整个 target 被跳过（不影响其他 target 编译）
# ============================================================

if(GPU_STUDY_NO_OPTIX OR NOT OPTIX_INCLUDE_DIR)
    message(STATUS "[CAPSTONE2_C] OptiX 不可用 — 跳过渲染器构建（其他 target 不受影响）")
    add_custom_target(CAPSTONE2_C_optix_mini_renderer
        COMMAND ${CMAKE_COMMAND} -E echo
            "[CAPSTONE2_C] 已跳过（OptiX 不可用）。请安装 OptiX SDK 8.x 后重新配置 CMake。"
        COMMENT "CAPSTONE2_C: OptiX 不可用，已跳过")
    return()
endif()

message(STATUS "[CAPSTONE2_C] 复用根 OPTIX_INCLUDE_DIR=${OPTIX_INCLUDE_DIR}")

# ── 编译 device programs（programs.cu → PTX / OPTIXIR）──────────
# OptiX 设备程序需要单独编译为 PTX 或 OptiX IR（.optixir），
# 再由 host 代码通过 optixModuleCreate 加载。
#
# CUDA_ARCHITECTURES "native" 让 nvcc 自动探测当前 GPU；
# 也可以显式指定 "75;80;86;89;90" 以生成 fat binary。
set(PROGRAMS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/programs.cu")
set(PROGRAMS_OPTIXIR "${CMAKE_CURRENT_BINARY_DIR}/programs.optixir")

# nvcc 编译为 OptiX IR（--optix-ir 需要 CUDA 11.7+ / OptiX 7.4+）
add_custom_command(
    OUTPUT  "${PROGRAMS_OPTIXIR}"
    COMMAND ${CMAKE_CUDA_COMPILER}
            -I "${OPTIX_INCLUDE_DIR}"
            -I "${CMAKE_SOURCE_DIR}/common"
            --optix-ir
            --expt-relaxed-constexpr
            --extended-lambda
            -std=c++20
            -arch=native
            -o "${PROGRAMS_OPTIXIR}"
            "${PROGRAMS_SRC}"
    DEPENDS "${PROGRAMS_SRC}"
    COMMENT "[CAPSTONE2_C] 编译 OptiX device programs → programs.optixir"
    VERBATIM)

add_custom_target(CAPSTONE2_C_programs DEPENDS "${PROGRAMS_OPTIXIR}")

# ── Host 可执行文件 ────────────────────────────────────────────
add_executable(CAPSTONE2_C_optix_mini_renderer host.cpp)

add_dependencies(CAPSTONE2_C_optix_mini_renderer CAPSTONE2_C_programs)

target_link_libraries(CAPSTONE2_C_optix_mini_renderer PRIVATE
    optix::headers
    gpu_study_common)

# 传递 .optixir 文件路径到 C++ 编译单元
target_compile_definitions(CAPSTONE2_C_optix_mini_renderer PRIVATE
    OPTIX_PROGRAMS_PTX_PATH="${PROGRAMS_OPTIXIR}")

if(MSVC)
    target_compile_options(CAPSTONE2_C_optix_mini_renderer PRIVATE
        /utf-8 /W3)
endif()

message(STATUS "[CAPSTONE2_C] 配置完成。OptiX root: ${OPTIX_ROOT}")
message(STATUS "[CAPSTONE2_C] Device programs output: ${PROGRAMS_OPTIXIR}")
