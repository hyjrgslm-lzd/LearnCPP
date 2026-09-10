cmake_minimum_required(VERSION 3.28)
project(L08_type_lists LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L08_type_lists
    HEADER type_list_tools.hpp
    CHECK checks/type_list_checks.cpp
    BAD_DIAGNOSTIC "error and stopped signatures must survive value transform"
)

c04_add_observation(L08_type_lists_observation observations/type_lists_observation.cpp)

c04_add_compile_case(
    NAME L08_lazy_type_eager_bad
    SOURCE validation/diagnostics/lazy_type_eager_bad.cpp
    CONTROL validation/diagnostics/lazy_type_control.cpp
    PATTERN "missing_type|C2039"
)
