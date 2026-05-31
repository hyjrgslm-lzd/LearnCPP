cmake_minimum_required(VERSION 3.28)
project(D3_warp_reduce_scan LANGUAGES CXX CUDA)

add_executable(D3_warp_reduce_scan main.cu)
target_link_libraries(D3_warp_reduce_scan PRIVATE gpu_study_common)
set_target_properties(D3_warp_reduce_scan PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)
