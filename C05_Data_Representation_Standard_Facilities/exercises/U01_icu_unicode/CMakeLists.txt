cmake_minimum_required(VERSION 3.28)
project(U01_icu_unicode LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

if(NOT DATA_STUDY_ENABLE_ICU)
    message(STATUS "U01_icu_unicode disabled: set DATA_STUDY_ENABLE_ICU=ON after running tools/prepare_icu.ps1")
    return()
endif()

include(../cmake/ICU.cmake)

add_executable(U01_icu_unicode_check checks/icu_unicode_checks.cpp)
c05_configure_target(U01_icu_unicode_check)
c05_link_icu(U01_icu_unicode_check)
target_compile_definitions(U01_icu_unicode_check PRIVATE
    C05_U01_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
c05_add_test(U01_icu_unicode_check TIMEOUT 180 LABELS icu)
