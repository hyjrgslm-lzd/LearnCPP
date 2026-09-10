cmake_minimum_required(VERSION 3.28)
project(L07_chained_hash LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME L07_chained_hash
    HEADER chained_hash.hpp
    CHECK checks/hash_checks.cpp
    BAD_DIAGNOSTIC "rehash keeps colliding keys"
)
