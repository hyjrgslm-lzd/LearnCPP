cmake_minimum_required(VERSION 3.28)
project(J1_cublas_and_cublaslt LANGUAGES CXX CUDA)

add_executable(J1_cublas_and_cublaslt main.cu)
target_link_libraries(J1_cublas_and_cublaslt PRIVATE gpu_study_common)
set_target_properties(J1_cublas_and_cublaslt PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# cuBLAS / cuBLASLt 随 CUDA Toolkit 附带，通常总是存在
target_link_libraries(J1_cublas_and_cublaslt PRIVATE CUDA::cublas CUDA::cublasLt)
target_compile_definitions(J1_cublas_and_cublaslt PRIVATE HAVE_CUBLAS=1)
