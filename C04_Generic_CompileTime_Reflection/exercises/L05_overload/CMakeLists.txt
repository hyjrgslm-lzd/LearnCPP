cmake_minimum_required(VERSION 3.28)
project(L05_overload LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L05_overload
    HEADER describe.hpp
    CHECK checks/describe_checks.cpp
    BAD_DIAGNOSTIC "string literal overload must keep the array extent"
)

c04_add_observation(L05_overload_observation observations/overload_observation.cpp)

c04_add_compile_case(
    NAME L05_func_partial
    SOURCE validation/diagnostics/function_partial_specialization.cpp
    CONTROL validation/diagnostics/function_partial_specialization_control.cpp
    PATTERN "error C2768|error C2912"
)

c04_add_compile_case(
    NAME L05_body_error
    SOURCE validation/diagnostics/sfinae_body_hard_error.cpp
    CONTROL validation/diagnostics/sfinae_immediate_context_control.cpp
    PATTERN "error C2039"
)
