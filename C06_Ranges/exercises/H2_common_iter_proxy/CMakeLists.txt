cmake_minimum_required(VERSION 3.28)
project(H2_common_iter_proxy LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME H2_common_iter_proxy
    HEADER common_proxy.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "iter_swap must exchange underlying elements"
)
