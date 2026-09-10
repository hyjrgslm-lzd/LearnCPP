cmake_minimum_required(VERSION 3.28)
project(L04_mapping LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c07_add_exercise(NAME L04_mapping HEADER mapping_lab.hpp
    CHECK checks/mapping_checks.cpp
    BAD_DIAGNOSTIC "readonly mapping aligns view offset down")
