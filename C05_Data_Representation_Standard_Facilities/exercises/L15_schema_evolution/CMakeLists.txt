cmake_minimum_required(VERSION 3.28)
project(L15_schema_evolution LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c05_add_exercise(
    NAME L15_schema_evolution
    HEADER schema_evolution.hpp
    CHECK checks/schema_checks.cpp
    BAD_DIAGNOSTIC "v1 writer rejects note")
