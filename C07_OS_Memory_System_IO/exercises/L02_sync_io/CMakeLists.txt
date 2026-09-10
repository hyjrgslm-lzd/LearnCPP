cmake_minimum_required(VERSION 3.28)
project(L02_sync_io LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c07_add_exercise(NAME L02_sync_io HEADER sync_io.hpp CHECK checks/sync_io_checks.cpp
    BAD_DIAGNOSTIC "copy reports every byte")
