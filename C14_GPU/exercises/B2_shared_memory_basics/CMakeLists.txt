cmake_minimum_required(VERSION 3.28)
project(B2_shared_memory_basics LANGUAGES CXX CUDA)

add_executable(B2_shared_memory_basics main.cu)
target_link_libraries(B2_shared_memory_basics PRIVATE gpu_study_common)
set_target_properties(B2_shared_memory_basics PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
