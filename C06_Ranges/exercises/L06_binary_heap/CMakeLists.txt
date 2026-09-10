cmake_minimum_required(VERSION 3.28)
project(L06_binary_heap LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME L06_binary_heap
    HEADER finite_max_heap.hpp
    CHECK checks/heap_checks.cpp
    BAD_DIAGNOSTIC "pop returns descending order"
)
