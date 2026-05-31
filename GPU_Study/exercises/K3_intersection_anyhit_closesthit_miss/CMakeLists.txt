cmake_minimum_required(VERSION 3.28)
project(K3_intersection_anyhit_closesthit_miss LANGUAGES CXX CUDA)

if(NOT OPTIX_INCLUDE_DIR)
    gpu_study_optix_unavailable_stub(K3_intersection_anyhit_closesthit_miss host.cpp)
    return()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# 将 programs.cu 编译为 .optixir（包含全五种 program type）
# ─────────────────────────────────────────────────────────────────────────────
set(K3_OPTIXIR "${CMAKE_CURRENT_BINARY_DIR}/programs.optixir")

add_custom_command(
    OUTPUT  "${K3_OPTIXIR}"
    COMMAND "${CMAKE_CUDA_COMPILER}"
            -I"${OPTIX_INCLUDE_DIR}"
            --optix-ir
            -std=c++17
            -arch=sm_75
            --use_fast_math
            -o "${K3_OPTIXIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/programs.cu"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/programs.cu"
    COMMENT "K3: 编译 OptiX IR (programs.cu -> programs.optixir)")

add_custom_target(K3_programs_optixir DEPENDS "${K3_OPTIXIR}")

# ─────────────────────────────────────────────────────────────────────────────
# Host 可执行文件
# ─────────────────────────────────────────────────────────────────────────────
add_executable(K3_intersection_anyhit_closesthit_miss host.cpp)

target_link_libraries(K3_intersection_anyhit_closesthit_miss PRIVATE
    optix::headers
    gpu_study_common
    CUDA::cudart
    CUDA::cuda_driver)

add_dependencies(K3_intersection_anyhit_closesthit_miss K3_programs_optixir)

target_compile_definitions(K3_intersection_anyhit_closesthit_miss PRIVATE
    K3_OPTIXIR_PATH="${K3_OPTIXIR}"
    CUDA_VERSION=1)
