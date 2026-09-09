cmake_minimum_required(VERSION 3.28)
project(I2_layernorm_rmsnorm_welford LANGUAGES CXX CUDA)

# sm_80+ 即可（FP16 Tensor Core 从 Ampere 起稳定支持）
add_executable(I2_layernorm_rmsnorm_welford main.cu)
target_link_libraries(I2_layernorm_rmsnorm_welford PRIVATE gpu_study_common)
set_target_properties(I2_layernorm_rmsnorm_welford PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "80;86;89;90a")
