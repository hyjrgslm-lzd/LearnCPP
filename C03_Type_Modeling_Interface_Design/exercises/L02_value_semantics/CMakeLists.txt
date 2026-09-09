cmake_minimum_required(VERSION 3.28)
project(L02_value_semantics LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c03_add_exercise(
    NAME L02_value_semantics
    HEADER money.hpp
    CHECK checks/money_checks.cpp
    BAD_DIAGNOSTIC "different currencies cannot be added")

