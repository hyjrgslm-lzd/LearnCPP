cmake_minimum_required(VERSION 3.28)
project(K1_optix_hello_pipeline LANGUAGES CXX CUDA)

# OptiX SDK 由根 cmake/OptiXSetup.cmake 统一发现
if(NOT OPTIX_INCLUDE_DIR)
    gpu_study_optix_unavailable_stub(K1_optix_hello_pipeline host.cpp)
    return()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# 将 programs.cu 编译为 .optixir（中间格式，不链接入 host 二进制）
# ─────────────────────────────────────────────────────────────────────────────
set(K1_OPTIXIR "${CMAKE_CURRENT_BINARY_DIR}/programs.optixir")

add_custom_command(
    OUTPUT  "${K1_OPTIXIR}"
    COMMAND "${CMAKE_CUDA_COMPILER}"
            -I"${OPTIX_INCLUDE_DIR}"
            --optix-ir
            -std=c++17
            -arch=sm_75
            --use_fast_math
            -o "${K1_OPTIXIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/programs.cu"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/programs.cu"
    COMMENT "K1: 编译 OptiX IR (programs.cu -> programs.optixir)")

add_custom_target(K1_programs_optixir DEPENDS "${K1_OPTIXIR}")

# ─────────────────────────────────────────────────────────────────────────────
# Host 可执行文件
# ─────────────────────────────────────────────────────────────────────────────
add_executable(K1_optix_hello_pipeline host.cpp)

target_link_libraries(K1_optix_hello_pipeline PRIVATE
    optix::headers
    gpu_study_common
    CUDA::cudart
    CUDA::cuda_driver)

add_dependencies(K1_optix_hello_pipeline K1_programs_optixir)

target_compile_definitions(K1_optix_hello_pipeline PRIVATE
    K1_OPTIXIR_PATH="${K1_OPTIXIR}"
    CUDA_VERSION=1)
