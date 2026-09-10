cmake_minimum_required(VERSION 3.28)
project(L04_lookup LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L04_lookup
    HEADER lookup_probe.hpp
    CHECK checks/lookup_checks.cpp
    BAD_DIAGNOSTIC "no-route object must not satisfy c04_inspectable"
)

c04_add_observation(L04_lookup_observation observations/lookup_observation.cpp)

c04_add_compile_case(
    NAME L04_lookup_missing_typename
    SOURCE validation/diagnostics/missing_typename.cpp
    CONTROL validation/diagnostics/missing_typename_control.cpp
    PATTERN "error C7510"
)
