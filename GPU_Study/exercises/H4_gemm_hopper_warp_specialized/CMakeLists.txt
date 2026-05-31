cmake_minimum_required(VERSION 3.28)
project(H4_gemm_hopper_warp_specialized LANGUAGES CXX CUDA)

# Hopper warp-specialized GEMM 要求 sm_90a：wgmma / TMA / mbarrier 仅在 sm_90a 下可用。
add_executable(H4_gemm_hopper_warp_specialized main.cu)
target_link_libraries(H4_gemm_hopper_warp_specialized PRIVATE gpu_study_common)
set_target_properties(H4_gemm_hopper_warp_specialized PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "90a")
