cmake_minimum_required(VERSION 3.28)
project(L01_invariants LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c03_add_exercise(
    NAME L01_invariants
    HEADER bounded_int.hpp
    CHECK checks/invariant_checks.cpp
    BAD_DIAGNOSTIC "constructor rejects values outside range")

