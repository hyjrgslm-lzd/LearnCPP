cmake_minimum_required(VERSION 3.28)
project(G4_tma_cp_async_bulk LANGUAGES CXX CUDA)

add_executable(G4_tma_cp_async_bulk main.cu)
target_link_libraries(G4_tma_cp_async_bulk PRIVATE gpu_study_common)
set_target_properties(G4_tma_cp_async_bulk PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)

# Hopper sm_90a required — TMA (Tensor Memory Accelerator) 与 cp.async.bulk.tensor 仅在 sm_90a 可用
set_target_properties(G4_tma_cp_async_bulk PROPERTIES CUDA_ARCHITECTURES "90a")
