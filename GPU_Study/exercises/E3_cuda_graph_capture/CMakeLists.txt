cmake_minimum_required(VERSION 3.28)
project(E3_cuda_graph_capture LANGUAGES CXX CUDA)

add_executable(E3_cuda_graph_capture main.cu)
target_link_libraries(E3_cuda_graph_capture PRIVATE gpu_study_common)
set_target_properties(E3_cuda_graph_capture PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
