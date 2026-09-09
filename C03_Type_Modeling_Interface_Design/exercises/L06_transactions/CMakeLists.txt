cmake_minimum_required(VERSION 3.28)
project(L06_transactions LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c03_add_exercise(
    NAME L06_transactions
    HEADER table.hpp
    CHECK checks/transaction_checks.cpp
    BAD_DIAGNOSTIC "strong failure keeps original table")

if(TYPE_STUDY_BUILD_REFERENCE)
    add_executable(L06_transactions_validation_bad_prefix checks/transaction_checks.cpp)
    c03_configure_target(L06_transactions_validation_bad_prefix)
    target_include_directories(L06_transactions_validation_bad_prefix PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad_prefix"
        "${CMAKE_CURRENT_SOURCE_DIR}/checks")
    c03_add_test(L06_transactions_validation_bad_prefix_rejected
        COMMAND ${CMAKE_COMMAND}
            "-DEXECUTABLE=$<TARGET_FILE:L06_transactions_validation_bad_prefix>"
            "-DEXPECTED_TEXT=strong prefix failure keeps original prefix"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/expect_failure.cmake"
        LABELS negative)
endif()
