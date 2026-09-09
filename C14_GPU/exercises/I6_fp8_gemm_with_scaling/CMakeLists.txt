cmake_minimum_required(VERSION 3.28)
project(I6_fp8_gemm_with_scaling LANGUAGES CXX CUDA)

# FP8 E4M3/E5M2 需要 sm_89+（Ada Lovelace）；sm_90a 推荐（Hopper wgmma FP8 路径）。
# 不使用全局 CMAKE_CUDA_ARCHITECTURES，而是显式覆盖：89 = Ada, 90a = Hopper。
add_executable(I6_fp8_gemm_with_scaling main.cu)
target_link_libraries(I6_fp8_gemm_with_scaling PRIVATE gpu_study_common)
set_target_properties(I6_fp8_gemm_with_scaling PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "89;90a")
