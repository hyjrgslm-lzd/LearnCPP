cmake_minimum_required(VERSION 3.28)
project(I1_softmax_online LANGUAGES CXX CUDA)

# sm_80+ 即可（Ampere/Ada/Hopper 均支持）
add_executable(I1_softmax_online main.cu)
target_link_libraries(I1_softmax_online PRIVATE gpu_study_common)
set_target_properties(I1_softmax_online PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "80;86;89;90a")
