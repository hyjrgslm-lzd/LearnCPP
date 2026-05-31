cmake_minimum_required(VERSION 3.28)
project(J4_triton_dsl_intro LANGUAGES CXX CUDA)

# J4 的核心是 Python Triton kernel；C++ 主程序仅打印使用说明
add_executable(J4_triton_dsl_intro main.cu)
target_link_libraries(J4_triton_dsl_intro PRIVATE gpu_study_common)
set_target_properties(J4_triton_dsl_intro PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
# 无额外外部 CUDA 库依赖
