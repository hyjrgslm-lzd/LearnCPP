cmake_minimum_required(VERSION 3.28)
project(F3_roofline_and_bottleneck LANGUAGES CXX CUDA)

add_executable(F3_roofline_and_bottleneck main.cu)
target_link_libraries(F3_roofline_and_bottleneck PRIVATE gpu_study_common)
set_target_properties(F3_roofline_and_bottleneck PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
