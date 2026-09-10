cmake_minimum_required(VERSION 3.28)
project(L13_configuration LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c05_add_exercise(
    NAME L13_configuration
    HEADER config_parser.hpp
    CHECK checks/config_checks.cpp
    BAD_DIAGNOSTIC "duplicate package_file")
