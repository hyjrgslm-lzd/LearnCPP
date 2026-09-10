cmake_minimum_required(VERSION 3.28)
project(H1_non_propagating_cache LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME H1_non_propagating_cache
    HEADER filter_cache.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "copy must rescan after non-propagating cache reset"
)
