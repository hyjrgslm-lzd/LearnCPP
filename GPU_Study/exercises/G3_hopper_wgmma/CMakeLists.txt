cmake_minimum_required(VERSION 3.28)
project(G3_hopper_wgmma LANGUAGES CXX CUDA)

add_executable(G3_hopper_wgmma main.cu)
target_link_libraries(G3_hopper_wgmma PRIVATE gpu_study_common)
set_target_properties(G3_hopper_wgmma PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)

# Hopper sm_90a required — wgmma 为 sm_90a 专有异步 warp-group 矩阵乘加指令
set_target_properties(G3_hopper_wgmma PROPERTIES CUDA_ARCHITECTURES "90a")
