cmake_minimum_required(VERSION 3.28)
project(L11_customization LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L11_customization
    HEADER read_value.hpp
    CHECK checks/read_value_checks.cpp
    BAD_DIAGNOSTIC "no-path object must not satisfy c04_readable"
)

c04_add_observation(L11_customization_baseline observations/baseline.cpp)
c04_add_observation(L11_customization_counterexamples observations/counterexamples.cpp)
