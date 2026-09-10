cmake_minimum_required(VERSION 3.28)
project(A01_compiletime_values LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME A01_compiletime_values
    HEADER compiletime_values.hpp
    CHECK checks/compiletime_values_checks.cpp
    BAD_DIAGNOSTIC "duplicate keys keep the first value before sorting"
)

c04_add_compile_case(
    NAME A01_diag_escape_pointer
    SOURCE validation/diagnostics/escape_pointer.cpp
    CONTROL validation/diagnostics/escape_control.cpp
    PATTERN "(constant expression|不是常量表达式|不是常数|not a constant expression)"
    SETUP validation/diagnostics/setup.cmake
)

c04_add_compile_case(
    NAME A01_diag_key_too_long
    SOURCE validation/diagnostics/key_too_long.cpp
    CONTROL validation/diagnostics/key_boundary_control.cpp
    PATTERN "(literal key is too long|static assertion failed|static_assert|静态断言)"
    SETUP validation/diagnostics/setup.cmake
)

c04_add_compile_case(
    NAME A01_diag_key_non_ascii
    SOURCE validation/diagnostics/key_non_ascii.cpp
    CONTROL validation/diagnostics/key_boundary_control.cpp
    PATTERN "(literal key|constant expression|不是常量表达式|不是常数|not a constant expression)"
    SETUP validation/diagnostics/setup.cmake
)

c04_add_compile_case(
    NAME A01_diag_key_embedded_nul
    SOURCE validation/diagnostics/key_embedded_nul.cpp
    CONTROL validation/diagnostics/key_boundary_control.cpp
    PATTERN "(literal key|constant expression|不是常量表达式|不是常数|not a constant expression)"
    SETUP validation/diagnostics/setup.cmake
)
