cmake_minimum_required(VERSION 3.28)
project(E1_streams_and_events LANGUAGES CXX CUDA)

add_executable(E1_streams_and_events main.cu)
target_link_libraries(E1_streams_and_events PRIVATE gpu_study_common)
set_target_properties(E1_streams_and_events PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
