cmake_minimum_required(VERSION 3.28)
project(CAPSTONE1_log_pipeline LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME CAPSTONE1_log_pipeline
    HEADER log_pipeline.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "valid records must parse each input line once"
)

foreach(variant IN ITEMS reference validation_good validation_bad)
    if(TARGET CAPSTONE1_log_pipeline_${variant})
        target_sources(CAPSTONE1_log_pipeline_${variant} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include/log_pipeline_data.hpp")
    endif()
endforeach()


learncpp_setup_ide(CAPSTONE1_log_pipeline_student)
