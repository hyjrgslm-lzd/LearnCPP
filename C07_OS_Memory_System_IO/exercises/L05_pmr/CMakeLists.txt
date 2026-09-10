cmake_minimum_required(VERSION 3.28)
project(L05_pmr LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c07_add_exercise(NAME L05_pmr HEADER pmr_lab.hpp
    CHECK checks/pmr_checks.cpp
    BAD_DIAGNOSTIC "fixed pool reuses freed blocks")
