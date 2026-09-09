cmake_minimum_required(VERSION 3.28)
project(H5_cutlass_collective_builder LANGUAGES CXX CUDA)

# CUTLASS CollectiveBuilder + Hopper warp-specialized schedule 要求 sm_90a。
add_executable(H5_cutlass_collective_builder main.cu)
target_link_libraries(H5_cutlass_collective_builder PRIVATE gpu_study_common cutlass::headers)
set_target_properties(H5_cutlass_collective_builder PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "90a")
