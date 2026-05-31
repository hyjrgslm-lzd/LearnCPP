cmake_minimum_required(VERSION 3.28)
project(C3_cooperative_groups LANGUAGES CXX CUDA)

add_executable(C3_cooperative_groups main.cu)
target_link_libraries(C3_cooperative_groups PRIVATE gpu_study_common)
set_target_properties(C3_cooperative_groups PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)
