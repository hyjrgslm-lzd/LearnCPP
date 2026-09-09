cmake_minimum_required(VERSION 3.28)
project(E2_async_mem_pool LANGUAGES CXX CUDA)

add_executable(E2_async_mem_pool main.cu)
target_link_libraries(E2_async_mem_pool PRIVATE gpu_study_common)
set_target_properties(E2_async_mem_pool PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
