cmake_minimum_required(VERSION 3.28)
project(CAPSTONE1_image_pipeline LANGUAGES CXX CUDA)

add_executable(CAPSTONE1_image_pipeline main.cu)
target_link_libraries(CAPSTONE1_image_pipeline PRIVATE gpu_study_common)
set_target_properties(CAPSTONE1_image_pipeline PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
# --expt-extended-lambda 允许在 device 代码中使用扩展 lambda（Thrust/CUB 需要）
target_compile_options(CAPSTONE1_image_pipeline PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--expt-extended-lambda>
)
