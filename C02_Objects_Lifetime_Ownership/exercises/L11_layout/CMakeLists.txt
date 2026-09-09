cmake_minimum_required(VERSION 3.28)
project(L11_layout LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

add_executable(L11_layout_observation checks/observation_check.cpp)
core_configure_target(L11_layout_observation)
core_add_test(L11_layout_observation COMMAND L11_layout_observation LABELS observation TIMEOUT 30)
