cmake_minimum_required(VERSION 3.28)
project(D1_warp_shuffle_basics LANGUAGES CXX CUDA)

add_executable(D1_warp_shuffle_basics main.cu)
target_link_libraries(D1_warp_shuffle_basics PRIVATE gpu_study_common)
set_target_properties(D1_warp_shuffle_basics PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)
