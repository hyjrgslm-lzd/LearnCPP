cmake_minimum_required(VERSION 3.28)
project(L01_handles LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c07_add_exercise(NAME L01_handles HEADER handles.hpp CHECK checks/handles_checks.cpp
    BAD_DIAGNOSTIC "existing target is refused")
c07_add_observation(L01_handles_ownership checks/ownership_observation.cpp)
c07_add_observation(L01_thread_process checks/thread_process_observation.cpp)

