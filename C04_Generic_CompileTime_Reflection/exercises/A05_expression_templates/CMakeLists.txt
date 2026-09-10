cmake_minimum_required(VERSION 3.28)
project(A05_expression_templates LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(NAME A05_expression_templates HEADER expression_templates.hpp CHECK checks/expression_template_checks.cpp
    BAD_DIAGNOSTIC "assign materializes before writing to handle reverse alias")
c04_add_observation(A05_eager_numeric_baseline observations/eager_numeric_reference.cpp)
c04_add_observation(A05_direct_write_bad observations/direct_write_bad.cpp)

c04_add_compile_case(NAME A05_diag_size SOURCE validation/diagnostics/size_mismatch.cpp
    CONTROL validation/diagnostics/control.cpp PATTERN "C2676|C2672|constraints not satisfied|no matching"
    SETUP validation/diagnostics/setup.cmake)
