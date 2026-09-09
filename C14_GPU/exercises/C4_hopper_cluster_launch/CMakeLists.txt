cmake_minimum_required(VERSION 3.28)
project(C4_hopper_cluster_launch LANGUAGES CXX CUDA)

# Hopper cluster 要求 sm_90a：__cluster_dims__ 与分布式 smem 仅在 sm_90a 编译目标下可用。
# 不使用全局 CMAKE_CUDA_ARCHITECTURES，而是显式覆盖此 target。
add_executable(C4_hopper_cluster_launch main.cu)
target_link_libraries(C4_hopper_cluster_launch PRIVATE gpu_study_common)
set_target_properties(C4_hopper_cluster_launch PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "90a")
