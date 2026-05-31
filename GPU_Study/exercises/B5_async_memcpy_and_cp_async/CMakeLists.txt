cmake_minimum_required(VERSION 3.28)
project(B5_async_memcpy_and_cp_async LANGUAGES CXX CUDA)

add_executable(B5_async_memcpy_and_cp_async main.cu)
# libcu++ (<cuda/barrier> 等) 是 cccl 的一部分，CUDA 13.x 随附，header-only
target_link_libraries(B5_async_memcpy_and_cp_async PRIVATE gpu_study_common CUDA::cudart)
set_target_properties(B5_async_memcpy_and_cp_async PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
