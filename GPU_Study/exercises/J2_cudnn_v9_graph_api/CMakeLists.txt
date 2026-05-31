cmake_minimum_required(VERSION 3.28)
project(J2_cudnn_v9_graph_api LANGUAGES CXX CUDA)

add_executable(J2_cudnn_v9_graph_api main.cu)
target_link_libraries(J2_cudnn_v9_graph_api PRIVATE gpu_study_common)
set_target_properties(J2_cudnn_v9_graph_api PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# cuDNN 可选依赖 — 若未安装则跳过，运行时打印提示
find_package(CUDNN QUIET)
if(CUDNN_FOUND)
    target_link_libraries(J2_cudnn_v9_graph_api PRIVATE CUDNN::cudnn)
    target_compile_definitions(J2_cudnn_v9_graph_api PRIVATE HAVE_CUDNN=1)
    message(STATUS "J2: cuDNN found (${CUDNN_VERSION}), building with cuDNN support")
else()
    message(STATUS "J2: cuDNN not found -- building stub (runtime will print skip message)")
endif()
