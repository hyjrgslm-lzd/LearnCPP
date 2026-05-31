cmake_minimum_required(VERSION 3.28)
project(G2_mma_sync_ptx LANGUAGES CXX CUDA)

add_executable(G2_mma_sync_ptx main.cu)
target_link_libraries(G2_mma_sync_ptx PRIVATE gpu_study_common)
set_target_properties(G2_mma_sync_ptx PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)
