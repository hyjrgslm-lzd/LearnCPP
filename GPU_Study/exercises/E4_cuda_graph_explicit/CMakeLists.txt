cmake_minimum_required(VERSION 3.28)
project(E4_cuda_graph_explicit LANGUAGES CXX CUDA)

add_executable(E4_cuda_graph_explicit main.cu)
target_link_libraries(E4_cuda_graph_explicit PRIVATE gpu_study_common)
set_target_properties(E4_cuda_graph_explicit PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
