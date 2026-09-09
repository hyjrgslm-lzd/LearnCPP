cmake_minimum_required(VERSION 3.28)
project(H1_gemm_naive_and_tiled LANGUAGES CXX CUDA)

add_executable(H1_gemm_naive_and_tiled main.cu)
target_link_libraries(H1_gemm_naive_and_tiled PRIVATE gpu_study_common)
set_target_properties(H1_gemm_naive_and_tiled PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "80;86;89;90a")
