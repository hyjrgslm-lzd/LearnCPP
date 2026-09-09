cmake_minimum_required(VERSION 3.28)
project(E5_mps_and_concurrent_clients LANGUAGES CXX CUDA)

add_executable(E5_mps_and_concurrent_clients main.cu)
target_link_libraries(E5_mps_and_concurrent_clients PRIVATE gpu_study_common)
set_target_properties(E5_mps_and_concurrent_clients PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
