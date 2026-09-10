cmake_minimum_required(VERSION 3.28)
project(H3_generator_const_iter LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME H3_generator_const_iter
    HEADER generator_const_iter.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "move assignment must destroy the old coroutine frame"
)
