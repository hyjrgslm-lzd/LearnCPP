cmake_minimum_required(VERSION 3.28)
project(U01_mp11 LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

if(NOT GENERIC_STUDY_ENABLE_META_LIBS)
    message(STATUS "U01_mp11 disabled: set GENERIC_STUDY_ENABLE_META_LIBS=ON after running tools/prepare_meta_libraries.ps1")
    return()
endif()

include(../cmake/MetaLibraries.cmake)

c04_add_exercise(
    NAME U01_mp11
    HEADER mp11_schema_tools.hpp
    CHECK checks/mp11_schema_checks.cpp
    BAD_DIAGNOSTIC "dynamic field dispatch must reject out-of-range indexes"
)

foreach(target U01_mp11_student U01_mp11_reference U01_mp11_validation_good U01_mp11_validation_bad)
    if(TARGET ${target})
        c04_link_mp11(${target})
        target_include_directories(${target} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/checks"
            "${CMAKE_CURRENT_SOURCE_DIR}/../A02_field_projection/checks")
    endif()
endforeach()

c04_add_compile_case(
    NAME U01_mp11_missing_key_diagnostic
    SOURCE validation/diagnostics/missing_key.cpp
    CONTROL validation/diagnostics/control.cpp
    PATTERN "mp11 map key not found"
    SETUP validation/diagnostics/setup.cmake
)
