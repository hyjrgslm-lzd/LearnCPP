cmake_minimum_required(VERSION 3.28)
project(H2_gemm_warp_tile_and_mma LANGUAGES CXX CUDA)

add_executable(H2_gemm_warp_tile_and_mma main.cu)
target_link_libraries(H2_gemm_warp_tile_and_mma PRIVATE gpu_study_common)
set_target_properties(H2_gemm_warp_tile_and_mma PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "80;86;89;90a")
