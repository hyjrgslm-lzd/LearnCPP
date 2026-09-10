cmake_minimum_required(VERSION 3.28)
project(E1_my_begin_cpo LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME E1_my_begin_cpo
    HEADER my_begin.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "member path wins over ADL begin"
)
