cmake_minimum_required(VERSION 3.28)
project(J3_nccl_allreduce_and_topology LANGUAGES CXX CUDA)

add_executable(J3_nccl_allreduce_and_topology main.cu)
target_link_libraries(J3_nccl_allreduce_and_topology PRIVATE gpu_study_common)
set_target_properties(J3_nccl_allreduce_and_topology PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# NCCL 可选依赖 — 随 CUDA Toolkit 附带或单独安装
find_library(NCCL_LIB nccl)
if(NCCL_LIB)
    target_link_libraries(J3_nccl_allreduce_and_topology PRIVATE ${NCCL_LIB})
    target_compile_definitions(J3_nccl_allreduce_and_topology PRIVATE HAVE_NCCL=1)
    message(STATUS "J3: NCCL found at ${NCCL_LIB}")
else()
    message(STATUS "J3: NCCL not found -- building stub (runtime will print skip message)")
endif()
