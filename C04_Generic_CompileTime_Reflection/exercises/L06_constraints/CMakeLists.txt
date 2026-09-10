cmake_minimum_required(VERSION 3.28)
project(L06_constraints LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L06_constraints
    HEADER record_range.hpp
    CHECK checks/record_range_checks.cpp
    BAD_DIAGNOSTIC "single-pass range must not satisfy stable_field_range"
)

c04_add_observation(L06_constraints_observation observations/constraints_observation.cpp)

c04_add_compile_case(
    NAME L06_non_dep
    SOURCE validation/diagnostics/non_dependent_error.cpp
    CONTROL validation/diagnostics/non_dependent_error_control.cpp
    PATTERN "error C3861"
)
