cmake_minimum_required(VERSION 3.28)
project(A4_sanitizer_debug LANGUAGES CXX CUDA)

add_executable(A4_sanitizer_debug main.cu)
target_link_libraries(A4_sanitizer_debug PRIVATE gpu_study_common)
set_target_properties(A4_sanitizer_debug PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

# 建议：调试时启用 -g 和 --generate-line-info，
# 便于 compute-sanitizer 输出精确的源文件行号。
# target_compile_options(A4_sanitizer_debug PRIVATE
#     $<$<COMPILE_LANGUAGE:CUDA>:-g --generate-line-info>
# )
