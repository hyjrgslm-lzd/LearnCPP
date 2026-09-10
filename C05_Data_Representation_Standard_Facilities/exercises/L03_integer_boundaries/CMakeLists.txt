cmake_minimum_required(VERSION 3.28)
project(L03_integer_boundaries LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c05_add_exercise(
    NAME L03_integer_boundaries
    HEADER checked_int.hpp
    CHECK checks/checked_int_checks.cpp
    BAD_DIAGNOSTIC "rejects one-past u32 max")
