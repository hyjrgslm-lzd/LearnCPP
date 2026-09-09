cmake_minimum_required(VERSION 3.28)
project(J5_tensorrt_engine_build LANGUAGES CXX CUDA)

add_executable(J5_tensorrt_engine_build main.cu)
target_link_libraries(J5_tensorrt_engine_build PRIVATE gpu_study_common)
set_target_properties(J5_tensorrt_engine_build PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# TensorRT 可选依赖 — 需从 https://developer.nvidia.com/tensorrt 单独下载
find_library(NVINFER_LIB nvinfer)
find_library(NVINFER_PLUGIN_LIB nvinfer_plugin)
if(NVINFER_LIB AND NVINFER_PLUGIN_LIB)
    target_link_libraries(J5_tensorrt_engine_build PRIVATE ${NVINFER_LIB} ${NVINFER_PLUGIN_LIB})
    target_compile_definitions(J5_tensorrt_engine_build PRIVATE HAVE_TENSORRT=1)
    message(STATUS "J5: TensorRT found (nvinfer=${NVINFER_LIB})")
else()
    message(STATUS "J5: TensorRT not found -- building stub (runtime will print skip message)")
endif()
