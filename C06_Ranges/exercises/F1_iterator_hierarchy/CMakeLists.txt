cmake_minimum_required(VERSION 3.28)
project(F1_iterator_hierarchy LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME F1_iterator_hierarchy
    HEADER iterator_hierarchy.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "random access distance matches"
)
