cmake_minimum_required(VERSION 3.28)
project(L15_schema_evolution LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c05_add_exercise(
    NAME L15_schema_evolution
    HEADER schema_evolution.hpp
    CHECK checks/schema_checks.cpp
    BAD_DIAGNOSTIC "v1 writer rejects note")

foreach(variant IN ITEMS reference validation_good validation_bad)
    if(TARGET L15_schema_evolution_${variant})
        target_sources(L15_schema_evolution_${variant} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/golden.hpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/provided/v1_reader.hpp")
    endif()
endforeach()


learncpp_setup_ide(L15_schema_evolution_student)
