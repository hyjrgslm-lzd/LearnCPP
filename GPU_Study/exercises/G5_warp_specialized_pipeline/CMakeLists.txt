cmake_minimum_required(VERSION 3.28)
project(G5_warp_specialized_pipeline LANGUAGES CXX CUDA)

add_executable(G5_warp_specialized_pipeline main.cu)
target_link_libraries(G5_warp_specialized_pipeline PRIVATE gpu_study_common)
set_target_properties(G5_warp_specialized_pipeline PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)

# Hopper sm_90a required — warp-specialized producer-consumer pipeline 依赖 wgmma + TMA，均为 sm_90a 专属
set_target_properties(G5_warp_specialized_pipeline PROPERTIES CUDA_ARCHITECTURES "90a")
