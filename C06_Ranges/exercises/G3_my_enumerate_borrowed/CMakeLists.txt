cmake_minimum_required(VERSION 3.28)
project(G3_my_enumerate_borrowed LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME G3_my_enumerate_borrowed
    HEADER my_enumerate_view.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "borrowed range is forwarded"
)
