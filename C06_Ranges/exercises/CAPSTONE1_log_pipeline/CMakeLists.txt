cmake_minimum_required(VERSION 3.28)
project(CAPSTONE1_log_pipeline LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME CAPSTONE1_log_pipeline
    HEADER log_pipeline.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "valid records must parse each input line once"
)
