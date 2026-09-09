cmake_minimum_required(VERSION 3.28)
project(F1_nsight_systems_timeline LANGUAGES CXX CUDA)

add_executable(F1_nsight_systems_timeline main.cu)
target_link_libraries(F1_nsight_systems_timeline PRIVATE gpu_study_common)
set_target_properties(F1_nsight_systems_timeline PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
