cmake_minimum_required(VERSION 3.28)
project(J6_transformer_engine_fp8 LANGUAGES CXX CUDA)

# J6 的核心是 Python TransformerEngine；C++ 主程序仅打印使用说明和硬件信息
add_executable(J6_transformer_engine_fp8 main.cu)
target_link_libraries(J6_transformer_engine_fp8 PRIVATE gpu_study_common)
set_target_properties(J6_transformer_engine_fp8 PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
# 无额外外部 CUDA 库依赖
