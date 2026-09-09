cmake_minimum_required(VERSION 3.28)
project(H3_gemm_pipelined_double_buffer LANGUAGES CXX CUDA)

add_executable(H3_gemm_pipelined_double_buffer main.cu)
target_link_libraries(H3_gemm_pipelined_double_buffer PRIVATE gpu_study_common)
set_target_properties(H3_gemm_pipelined_double_buffer PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "80;86;89;90a")
