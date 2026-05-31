cmake_minimum_required(VERSION 3.28)
project(K4_optix_denoiser_and_cuda_interop LANGUAGES CXX CUDA)

if(NOT OPTIX_INCLUDE_DIR)
    gpu_study_optix_unavailable_stub(K4_optix_denoiser_and_cuda_interop host.cpp)
    return()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# 将 programs.cu 编译为 .optixir（路径追踪 + 法线/albedo 辅助 buffer）
# ─────────────────────────────────────────────────────────────────────────────
set(K4_OPTIXIR "${CMAKE_CURRENT_BINARY_DIR}/programs.optixir")

add_custom_command(
    OUTPUT  "${K4_OPTIXIR}"
    COMMAND "${CMAKE_CUDA_COMPILER}"
            -I"${OPTIX_INCLUDE_DIR}"
            --optix-ir
            -std=c++17
            -arch=sm_75
            --use_fast_math
            -o "${K4_OPTIXIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/programs.cu"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/programs.cu"
    COMMENT "K4: 编译 OptiX IR (programs.cu -> programs.optixir)")

add_custom_target(K4_programs_optixir DEPENDS "${K4_OPTIXIR}")

# ─────────────────────────────────────────────────────────────────────────────
# Host 可执行文件（包含 CUDA tonemap kernel 的 .cpp 编译单元）
# ─────────────────────────────────────────────────────────────────────────────
add_executable(K4_optix_denoiser_and_cuda_interop host.cpp)

target_link_libraries(K4_optix_denoiser_and_cuda_interop PRIVATE
    optix::headers
    gpu_study_common
    CUDA::cudart
    CUDA::cuda_driver)

add_dependencies(K4_optix_denoiser_and_cuda_interop K4_programs_optixir)

target_compile_definitions(K4_optix_denoiser_and_cuda_interop PRIVATE
    K4_OPTIXIR_PATH="${K4_OPTIXIR}"
    CUDA_VERSION=1)
