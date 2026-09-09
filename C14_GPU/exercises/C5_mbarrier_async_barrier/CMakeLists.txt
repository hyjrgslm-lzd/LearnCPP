cmake_minimum_required(VERSION 3.28)
project(C5_mbarrier_async_barrier LANGUAGES CXX CUDA)

add_executable(C5_mbarrier_async_barrier main.cu)
target_link_libraries(C5_mbarrier_async_barrier PRIVATE gpu_study_common)
set_target_properties(C5_mbarrier_async_barrier PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)
