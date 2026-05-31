cmake_minimum_required(VERSION 3.28)
project(B6_texture_and_constant LANGUAGES CXX CUDA)

add_executable(B6_texture_and_constant main.cu)
target_link_libraries(B6_texture_and_constant PRIVATE gpu_study_common)
set_target_properties(B6_texture_and_constant PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
