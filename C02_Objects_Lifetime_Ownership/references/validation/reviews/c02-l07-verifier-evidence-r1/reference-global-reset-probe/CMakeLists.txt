cmake_minimum_required(VERSION 3.28)
project(c02_l07_reference_global_reset_probe LANGUAGES CXX)
include("${CMAKE_CURRENT_LIST_DIR}/../../../../../exercises/cmake/StudySetup.cmake")
add_executable(c02_l07_reference_global_reset_probe main.cpp)
target_include_directories(c02_l07_reference_global_reset_probe PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../../../../exercises/L07_raii/src/reference")
core_configure_target(c02_l07_reference_global_reset_probe)
