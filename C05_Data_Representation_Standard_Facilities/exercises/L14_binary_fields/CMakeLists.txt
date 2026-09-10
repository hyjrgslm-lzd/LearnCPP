cmake_minimum_required(VERSION 3.28)
project(L14_binary_fields LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c05_add_exercise(
    NAME L14_binary_fields
    HEADER bounded_field.hpp
    CHECK checks/field_checks.cpp
    BAD_DIAGNOSTIC "truncated payload keeps cursor")
