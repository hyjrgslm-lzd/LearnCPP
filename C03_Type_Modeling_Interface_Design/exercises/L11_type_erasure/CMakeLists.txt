cmake_minimum_required(VERSION 3.28)
project(L11_type_erasure LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c03_add_exercise(
    NAME L11_type_erasure
    HEADER any_shape.hpp
    CHECK checks/any_shape_checks.cpp
    BAD_DIAGNOSTIC "copy failure keeps target unchanged")

if(TYPE_STUDY_BUILD_REFERENCE)
    add_executable(L11_type_erasure_validation_bad_empty checks/any_shape_checks.cpp)
    c03_configure_target(L11_type_erasure_validation_bad_empty)
    target_include_directories(L11_type_erasure_validation_bad_empty PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad_empty"
        "${CMAKE_CURRENT_SOURCE_DIR}/checks")
    c03_add_test(L11_type_erasure_validation_bad_empty_rejected
        COMMAND ${CMAKE_COMMAND}
            "-DEXECUTABLE=$<TARGET_FILE:L11_type_erasure_validation_bad_empty>"
            "-DEXPECTED_TEXT=empty name throws"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/expect_failure.cmake"
        LABELS negative)
endif()

