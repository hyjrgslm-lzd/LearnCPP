cmake_minimum_required(VERSION 3.28)
project(G2_my_transform_closure LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME G2_my_transform_closure
    HEADER my_transform_view.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "dereference does not copy the callable"
)
