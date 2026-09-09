cmake_minimum_required(VERSION 3.28)
project(I5_flash_attention_v2_style LANGUAGES CXX CUDA)

# sm_80+ 满足 mma.sync FP16 Tensor Core；sm_90a 推荐（wgmma + TMA 进阶）
add_executable(I5_flash_attention_v2_style main.cu)
target_link_libraries(I5_flash_attention_v2_style PRIVATE gpu_study_common)
set_target_properties(I5_flash_attention_v2_style PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "80;86;89;90a")
