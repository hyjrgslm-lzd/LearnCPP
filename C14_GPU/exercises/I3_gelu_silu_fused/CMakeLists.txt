cmake_minimum_required(VERSION 3.28)
project(I3_gelu_silu_fused LANGUAGES CXX CUDA)

# sm_80+ 即可；sm_90a 可利用 __nv_bfloat16 向量内建函数进阶加速
add_executable(I3_gelu_silu_fused main.cu)
target_link_libraries(I3_gelu_silu_fused PRIVATE gpu_study_common)
set_target_properties(I3_gelu_silu_fused PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "80;86;89;90a")
