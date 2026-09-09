cmake_minimum_required(VERSION 3.28)
project(c02_l07_r3_fake_support_shadow LANGUAGES CXX)
include("${CMAKE_CURRENT_LIST_DIR}/../../../../../exercises/cmake/StudySetup.cmake")
add_executable(c02_l07_r3_fake_support_shadow
    "${CMAKE_CURRENT_LIST_DIR}/../../../../../exercises/L07_raii/checks/validation_check.cpp")
target_include_directories(c02_l07_r3_fake_support_shadow PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../../exercises/L07_raii/checks/support"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../../exercises/L07_raii/checks")
core_configure_target(c02_l07_r3_fake_support_shadow)
