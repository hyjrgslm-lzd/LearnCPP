cmake_minimum_required(VERSION 3.28)
project(L02_deduction LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L02_deduction
    HEADER deduction_probe.hpp
    CHECK checks/deduction_checks.cpp
    BAD_DIAGNOSTIC "array reference must preserve extent"
)

c04_add_observation(L02_deduction_decltype observations/decltype_rules.cpp)
