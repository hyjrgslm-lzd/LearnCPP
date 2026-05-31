cmake_minimum_required(VERSION 3.28)
project(I4_attention_fwd_unfused_vs_fused LANGUAGES CXX CUDA)

# sm_80+ 即可（FP16 mma.sync 从 Ampere 起可用）
add_executable(I4_attention_fwd_unfused_vs_fused main.cu)
target_link_libraries(I4_attention_fwd_unfused_vs_fused PRIVATE gpu_study_common)
set_target_properties(I4_attention_fwd_unfused_vs_fused PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "80;86;89;90a")
