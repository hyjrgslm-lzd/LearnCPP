cmake_minimum_required(VERSION 3.28)
project(L13_aliasing LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

add_executable(L13_aliasing_observation checks/observation_check.cpp)
core_configure_target(L13_aliasing_observation)
core_add_test(L13_aliasing_observation COMMAND L13_aliasing_observation LABELS observation TIMEOUT 30)
