cmake_minimum_required(VERSION 3.28)
project(A1_hello_device LANGUAGES CXX CUDA)

add_executable(A1_hello_device main.cu)
target_link_libraries(A1_hello_device PRIVATE gpu_study_common)
set_target_properties(A1_hello_device PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
