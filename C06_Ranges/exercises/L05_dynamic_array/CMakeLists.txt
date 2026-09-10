cmake_minimum_required(VERSION 3.28)
project(L05_dynamic_array LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME L05_dynamic_array
    HEADER dynamic_array.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "push growth failure keeps old capacity"
)
