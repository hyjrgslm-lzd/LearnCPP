cmake_minimum_required(VERSION 3.28)
project(L03_forwarding LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L03_forwarding
    HEADER forwarding_tools.hpp
    CHECK checks/forwarding_checks.cpp
    BAD_DIAGNOSTIC "invoke_preserving must keep lvalue argument category"
)

c04_add_observation(L03_forwarding_counterexamples observations/counterexamples.cpp)
