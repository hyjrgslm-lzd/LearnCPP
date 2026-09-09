cmake_minimum_required(VERSION 3.28)
project(L09_dynamic_polymorphism LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c03_add_exercise(
    NAME L09_dynamic_polymorphism
    HEADER shape.hpp
    CHECK checks/dynamic_shape_checks.cpp
    BAD_DIAGNOSTIC "clone preserves dynamic type")

if(TYPE_STUDY_BUILD_REFERENCE)
    add_executable(L09_dynamic_polymorphism_validation_bad_alias checks/dynamic_shape_checks.cpp)
    c03_configure_target(L09_dynamic_polymorphism_validation_bad_alias)
    target_include_directories(L09_dynamic_polymorphism_validation_bad_alias PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad_alias"
        "${CMAKE_CURRENT_SOURCE_DIR}/checks")
    c03_add_test(L09_dynamic_polymorphism_validation_bad_alias_rejected
        COMMAND ${CMAKE_COMMAND}
            "-DEXECUTABLE=$<TARGET_FILE:L09_dynamic_polymorphism_validation_bad_alias>"
            "-DEXPECTED_TEXT=clone owns an independent object"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/expect_failure.cmake"
        LABELS negative)

    add_executable(L09_dynamic_polymorphism_validation_bad_wrong_value checks/dynamic_shape_checks.cpp)
    c03_configure_target(L09_dynamic_polymorphism_validation_bad_wrong_value)
    target_include_directories(L09_dynamic_polymorphism_validation_bad_wrong_value PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad_wrong_value"
        "${CMAKE_CURRENT_SOURCE_DIR}/checks")
    c03_add_test(L09_dynamic_polymorphism_validation_bad_wrong_value_rejected
        COMMAND ${CMAKE_COMMAND}
            "-DEXECUTABLE=$<TARGET_FILE:L09_dynamic_polymorphism_validation_bad_wrong_value>"
            "-DEXPECTED_TEXT=rectangle clone keeps observable dimensions"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/expect_failure.cmake"
        LABELS negative)
endif()
