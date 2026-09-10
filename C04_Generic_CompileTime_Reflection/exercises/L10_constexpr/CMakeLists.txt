cmake_minimum_required(VERSION 3.28)
project(L10_constexpr LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L10_constexpr
    HEADER constexpr_tools.hpp
    CHECK checks/constexpr_tools_checks.cpp
    BAD_DIAGNOSTIC "if consteval branch must distinguish runtime calls"
)

c04_add_observation(L10_constexpr_observation observations/constexpr_observation.cpp)

c04_add_compile_case(
    NAME L10_invalid_digit_diagnostic
    SOURCE validation/diagnostics/invalid_digit.cpp
    CONTROL validation/diagnostics/invalid_digit_control.cpp
    PATTERN "decimal_value accepts only decimal digits"
)

c04_add_compile_case(
    NAME L10_empty_decimal_diagnostic
    SOURCE validation/diagnostics/empty_decimal.cpp
    CONTROL validation/diagnostics/invalid_digit_control.cpp
    PATTERN "decimal_value accepts only decimal digits"
)

c04_add_compile_case(
    NAME L10_embedded_nul_diagnostic
    SOURCE validation/diagnostics/embedded_nul.cpp
    CONTROL validation/diagnostics/invalid_digit_control.cpp
    PATTERN "decimal_value accepts only decimal digits"
)

c04_add_compile_case(
    NAME L10_signed_decimal_diagnostic
    SOURCE validation/diagnostics/signed_decimal.cpp
    CONTROL validation/diagnostics/invalid_digit_control.cpp
    PATTERN "decimal_value accepts only decimal digits"
)

c04_add_compile_case(
    NAME L10_spaced_decimal_diagnostic
    SOURCE validation/diagnostics/spaced_decimal.cpp
    CONTROL validation/diagnostics/invalid_digit_control.cpp
    PATTERN "decimal_value accepts only decimal digits"
)

c04_add_compile_case(
    NAME L10_decimal_overflow_diagnostic
    SOURCE validation/diagnostics/decimal_overflow.cpp
    CONTROL validation/diagnostics/invalid_digit_control.cpp
    PATTERN "decimal_value exceeds int range"
)

c04_add_compile_case(
    NAME L10_parameter_constant_diagnostic
    SOURCE validation/diagnostics/parameter_constant.cpp
    CONTROL validation/diagnostics/parameter_constant_control.cpp
    PATTERN "constant|常量|常数"
)
