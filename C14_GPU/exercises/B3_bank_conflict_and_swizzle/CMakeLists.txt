cmake_minimum_required(VERSION 3.28)
project(B3_bank_conflict_and_swizzle LANGUAGES CXX CUDA)

add_executable(B3_bank_conflict_and_swizzle main.cu)
# CUB 是 cccl 的一部分，CUDA 13.x 随附，header-only，无需额外链接
target_link_libraries(B3_bank_conflict_and_swizzle PRIVATE gpu_study_common CUDA::cudart)
set_target_properties(B3_bank_conflict_and_swizzle PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
