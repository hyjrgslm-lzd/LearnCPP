cmake_minimum_required(VERSION 3.28)
project(B01_cost LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

if(NOT RANGES_BUILD_REFERENCE)
    message(STATUS "B01_cost skipped because RANGES_BUILD_REFERENCE=OFF")
    return()
endif()

add_executable(B01_cost main.cpp)
ranges_configure_target(B01_cost)
target_include_directories(B01_cost PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../CAPSTONE1_log_pipeline/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../CAPSTONE1_log_pipeline/src/reference")

ranges_add_test(B01_cost_check COMMAND B01_cost --check LABELS benchmark TIMEOUT 30)
