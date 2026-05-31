cmake_minimum_required(VERSION 3.28)
project(D4_ldg_and_read_only_cache LANGUAGES CXX CUDA)

add_executable(D4_ldg_and_read_only_cache main.cu)
target_link_libraries(D4_ldg_and_read_only_cache PRIVATE gpu_study_common)
set_target_properties(D4_ldg_and_read_only_cache PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)
