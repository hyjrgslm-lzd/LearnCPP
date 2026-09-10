cmake_minimum_required(VERSION 3.28)
project(L09_tuple LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L09_tuple
    HEADER tuple_for_each.hpp
    CHECK checks/tuple_for_each_checks.cpp
    BAD_DIAGNOSTIC "tuple traversal must preserve left-to-right order"
)

c04_add_observation(L09_tuple_observation observations/tuple_observation.cpp)
