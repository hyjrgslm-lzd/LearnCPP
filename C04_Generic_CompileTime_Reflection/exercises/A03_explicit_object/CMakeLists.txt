cmake_minimum_required(VERSION 3.28)
project(A03_explicit_object LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME A03_explicit_object
    HEADER explicit_object.hpp
    CHECK checks/explicit_object_checks.cpp
    BAD_DIAGNOSTIC "value preserves object identity and cvref category"
)

c04_add_compile_case(
    NAME A03_diag_const_take
    SOURCE validation/diagnostics/const_take.cpp
    CONTROL validation/diagnostics/take_control.cpp
    PATTERN "(cannot move from const slot|deleted|已删除)"
    SETUP validation/diagnostics/setup.cmake
)
