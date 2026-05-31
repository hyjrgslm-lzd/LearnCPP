cmake_minimum_required(VERSION 3.28)
project(G1_wmma_fp16_basic LANGUAGES CXX CUDA)

add_executable(G1_wmma_fp16_basic main.cu)
target_link_libraries(G1_wmma_fp16_basic PRIVATE gpu_study_common)
set_target_properties(G1_wmma_fp16_basic PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)
