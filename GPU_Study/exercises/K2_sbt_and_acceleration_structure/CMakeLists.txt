cmake_minimum_required(VERSION 3.28)
project(K2_sbt_and_acceleration_structure LANGUAGES CXX CUDA)

if(NOT OPTIX_INCLUDE_DIR)
    gpu_study_optix_unavailable_stub(K2_sbt_and_acceleration_structure host.cpp)
    return()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# 将 programs.cu 编译为 .optixir
# ─────────────────────────────────────────────────────────────────────────────
set(K2_OPTIXIR "${CMAKE_CURRENT_BINARY_DIR}/programs.optixir")

add_custom_command(
    OUTPUT  "${K2_OPTIXIR}"
    COMMAND "${CMAKE_CUDA_COMPILER}"
            -I"${OPTIX_INCLUDE_DIR}"
            --optix-ir
            -std=c++17
            -arch=sm_75
            --use_fast_math
            -o "${K2_OPTIXIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/programs.cu"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/programs.cu"
    COMMENT "K2: 编译 OptiX IR (programs.cu -> programs.optixir)")

add_custom_target(K2_programs_optixir DEPENDS "${K2_OPTIXIR}")

# ─────────────────────────────────────────────────────────────────────────────
# Host 可执行文件
# ─────────────────────────────────────────────────────────────────────────────
add_executable(K2_sbt_and_acceleration_structure host.cpp)

target_link_libraries(K2_sbt_and_acceleration_structure PRIVATE
    optix::headers
    gpu_study_common
    CUDA::cudart
    CUDA::cuda_driver)

add_dependencies(K2_sbt_and_acceleration_structure K2_programs_optixir)

target_compile_definitions(K2_sbt_and_acceleration_structure PRIVATE
    K2_OPTIXIR_PATH="${K2_OPTIXIR}"
    CUDA_VERSION=1)
