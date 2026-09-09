cmake_minimum_required(VERSION 3.28)
project(H6_cute_layout_and_tensor LANGUAGES CXX CUDA)

# cuTe mini-GEMM：依赖 CUTLASS headers（cute/ 子目录），sm_80+ 即可编译。
add_executable(H6_cute_layout_and_tensor main.cu)
target_link_libraries(H6_cute_layout_and_tensor PRIVATE gpu_study_common cutlass::headers)
set_target_properties(H6_cute_layout_and_tensor PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "80;86;89;90a")
