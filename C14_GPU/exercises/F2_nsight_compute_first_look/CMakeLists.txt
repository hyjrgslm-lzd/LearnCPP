cmake_minimum_required(VERSION 3.28)
project(F2_nsight_compute_first_look LANGUAGES CXX CUDA)

add_executable(F2_nsight_compute_first_look main.cu)
target_link_libraries(F2_nsight_compute_first_look PRIVATE gpu_study_common)
set_target_properties(F2_nsight_compute_first_look PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
