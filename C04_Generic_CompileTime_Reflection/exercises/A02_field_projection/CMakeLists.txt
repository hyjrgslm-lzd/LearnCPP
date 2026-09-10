cmake_minimum_required(VERSION 3.28)
project(A02_field_projection LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME A02_field_projection
    HEADER field_projection.hpp
    CHECK checks/field_projection_checks.cpp
    BAD_DIAGNOSTIC "project returns writable references in requested order"
)

c04_add_observation(A02_known_projection_baseline observations/known_projection_baseline.cpp)
c04_add_observation(A02_projection_counterexamples observations/projection_counterexamples.cpp)
target_include_directories(A02_known_projection_baseline PRIVATE checks)
target_include_directories(A02_projection_counterexamples PRIVATE checks)

c04_add_compile_case(
    NAME A02_diag_unknown
    SOURCE validation/diagnostics/unknown_field.cpp
    CONTROL validation/diagnostics/projection_control.cpp
    PATTERN "unknown projection field"
    SETUP validation/diagnostics/setup.cmake
)
c04_add_compile_case(
    NAME A02_diag_dup
    SOURCE validation/diagnostics/duplicate_field.cpp
    CONTROL validation/diagnostics/projection_control.cpp
    PATTERN "duplicate projection field"
    SETUP validation/diagnostics/setup.cmake
)
c04_add_compile_case(
    NAME A02_diag_empty
    SOURCE validation/diagnostics/empty_component.cpp
    CONTROL validation/diagnostics/projection_control.cpp
    PATTERN "projection component must not be empty"
    SETUP validation/diagnostics/setup.cmake
)
c04_add_compile_case(
    NAME A02_diag_bad_id
    SOURCE validation/diagnostics/invalid_identifier.cpp
    CONTROL validation/diagnostics/projection_control.cpp
    PATTERN "projection field name must be an ASCII identifier"
    SETUP validation/diagnostics/setup.cmake
)
c04_add_compile_case(
    NAME A02_diag_rvalue
    SOURCE validation/diagnostics/rvalue_project.cpp
    CONTROL validation/diagnostics/projection_control.cpp
    PATTERN "(已删除|deleted)"
    SETUP validation/diagnostics/setup.cmake
)
c04_add_compile_case(
    NAME A02_diag_trail
    SOURCE validation/diagnostics/trailing_component.cpp
    CONTROL validation/diagnostics/projection_control.cpp
    PATTERN "projection component must not be empty"
    SETUP validation/diagnostics/setup.cmake
)
c04_add_compile_case(
    NAME A02_diag_nul
    SOURCE validation/diagnostics/embedded_nul.cpp
    CONTROL validation/diagnostics/projection_control.cpp
    PATTERN "projection field name must be an ASCII identifier"
    SETUP validation/diagnostics/setup.cmake
)
