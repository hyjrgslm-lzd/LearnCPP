cmake_minimum_required(VERSION 3.28)
project(L15_evolution LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c03_add_exercise(
    NAME L15_evolution
    HEADER compat_adapter.hpp
    CHECK checks/evolution_checks.cpp
    BAD_DIAGNOSTIC "v1 default timeout remains 1000")
