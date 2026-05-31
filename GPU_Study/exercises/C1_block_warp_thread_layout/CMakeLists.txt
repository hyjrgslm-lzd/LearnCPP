cmake_minimum_required(VERSION 3.28)
project(C1_block_warp_thread_layout LANGUAGES CXX CUDA)

add_executable(C1_block_warp_thread_layout main.cu)
target_link_libraries(C1_block_warp_thread_layout PRIVATE gpu_study_common)
set_target_properties(C1_block_warp_thread_layout PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)
