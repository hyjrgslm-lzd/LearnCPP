cmake_minimum_required(VERSION 3.28)
project(CAPSTONE2_B_flash_attention_replica LANGUAGES CXX CUDA)

add_executable(CAPSTONE2_B_flash_attention_replica main.cu)
target_link_libraries(CAPSTONE2_B_flash_attention_replica PRIVATE gpu_study_common)
set_target_properties(CAPSTONE2_B_flash_attention_replica PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# sm_80+（mma.sync FP16）；sm_90a 上可叠 TMA 进阶路径
set_target_properties(CAPSTONE2_B_flash_attention_replica PROPERTIES CUDA_ARCHITECTURES "80;86;89;90a")
