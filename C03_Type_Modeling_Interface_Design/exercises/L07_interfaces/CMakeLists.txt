cmake_minimum_required(VERSION 3.28)
project(L07_interfaces LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c03_add_exercise(
    NAME L07_interfaces
    HEADER owner.hpp
    CHECK checks/interface_checks.cpp
    BAD_DIAGNOSTIC "snapshot remains independent after owner mutation")

if(TYPE_STUDY_BUILD_REFERENCE)
    add_executable(L07_interfaces_rvalue_view_rejected EXCLUDE_FROM_ALL checks/rvalue_view_negative.cpp)
    c03_configure_target(L07_interfaces_rvalue_view_rejected)
    target_include_directories(L07_interfaces_rvalue_view_rejected PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/reference"
        "${CMAKE_CURRENT_SOURCE_DIR}/checks")
    add_test(NAME L07_interfaces_rvalue_view_rejected
        COMMAND ${CMAKE_COMMAND}
            --build "${CMAKE_BINARY_DIR}"
            --config "$<CONFIG>"
            --target L07_interfaces_rvalue_view_rejected)
    set_tests_properties(L07_interfaces_rvalue_view_rejected PROPERTIES
        WILL_FAIL TRUE
        TIMEOUT 30
        LABELS negative)

    add_executable(L07_interfaces_validation_bad_rvalue checks/interface_checks.cpp)
    c03_configure_target(L07_interfaces_validation_bad_rvalue)
    target_include_directories(L07_interfaces_validation_bad_rvalue PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad_rvalue"
        "${CMAKE_CURRENT_SOURCE_DIR}/checks")
    c03_add_test(L07_interfaces_validation_bad_rvalue_rejected
        COMMAND ${CMAKE_COMMAND}
            "-DEXECUTABLE=$<TARGET_FILE:L07_interfaces_validation_bad_rvalue>"
            "-DEXPECTED_TEXT=rvalue view is rejected for selected implementation"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/expect_failure.cmake"
        LABELS negative
        TIMEOUT 30)
endif()
