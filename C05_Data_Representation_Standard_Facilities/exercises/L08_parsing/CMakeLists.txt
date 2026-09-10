cmake_minimum_required(VERSION 3.28)
project(L08_parsing LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c05_add_exercise(
    NAME L08_parsing
    HEADER text_parse.hpp
    CHECK checks/parse_checks.cpp
    BAD_DIAGNOSTIC "rejects trailing characters")
