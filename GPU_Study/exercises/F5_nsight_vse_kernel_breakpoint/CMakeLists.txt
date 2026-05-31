cmake_minimum_required(VERSION 3.28)
project(F5_nsight_vse_kernel_breakpoint LANGUAGES CXX CUDA)

add_executable(F5_nsight_vse_kernel_breakpoint main.cu)
target_link_libraries(F5_nsight_vse_kernel_breakpoint PRIVATE gpu_study_common)
set_target_properties(F5_nsight_vse_kernel_breakpoint PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON)

