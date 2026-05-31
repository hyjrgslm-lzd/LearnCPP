cmake_minimum_required(VERSION 3.28)
project(B1_global_coalescing LANGUAGES CXX CUDA)

add_executable(B1_global_coalescing main.cu)
target_link_libraries(B1_global_coalescing PRIVATE gpu_study_common)
set_target_properties(B1_global_coalescing PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
