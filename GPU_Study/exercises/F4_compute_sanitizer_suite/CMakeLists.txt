cmake_minimum_required(VERSION 3.28)
project(F4_compute_sanitizer_suite LANGUAGES CXX CUDA)

add_executable(F4_compute_sanitizer_suite main.cu)
target_link_libraries(F4_compute_sanitizer_suite PRIVATE gpu_study_common)
set_target_properties(F4_compute_sanitizer_suite PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
