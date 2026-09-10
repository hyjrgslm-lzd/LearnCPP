cmake_minimum_required(VERSION 3.28)
project(L03_virtual_memory LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c07_add_exercise(NAME L03_virtual_memory HEADER virtual_memory_lab.hpp
    CHECK checks/virtual_memory_checks.cpp
    BAD_DIAGNOSTIC "committed page keeps written bytes")
