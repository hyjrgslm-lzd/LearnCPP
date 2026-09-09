cmake_minimum_required(VERSION 3.28)
project(A2_index_mapping LANGUAGES CXX CUDA)

add_executable(A2_index_mapping main.cu)
target_link_libraries(A2_index_mapping PRIVATE gpu_study_common)
set_target_properties(A2_index_mapping PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
