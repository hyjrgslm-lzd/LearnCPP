cmake_minimum_required(VERSION 3.28)
project(A3_nvcc_ptx_sass LANGUAGES CXX CUDA)

add_executable(A3_nvcc_ptx_sass main.cu)
target_link_libraries(A3_nvcc_ptx_sass PRIVATE gpu_study_common)
set_target_properties(A3_nvcc_ptx_sass PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

# 可选：打开行号信息，便于 Nsight Compute / PTX 调试
# target_compile_options(A3_nvcc_ptx_sass PRIVATE
#     $<$<COMPILE_LANGUAGE:CUDA>:--generate-line-info>
# )
