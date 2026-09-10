cmake_minimum_required(VERSION 3.28)
project(CAPSTONE4_mini_ranges LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME CAPSTONE4_mini_ranges
    HEADER my_ranges/06_consumers.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "take_view stops after requested count"
)
