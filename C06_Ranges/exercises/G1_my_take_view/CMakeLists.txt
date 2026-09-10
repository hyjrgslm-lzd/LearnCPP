cmake_minimum_required(VERSION 3.28)
project(G1_my_take_view LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME G1_my_take_view
    HEADER my_take_view.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "negative count is rejected"
)
