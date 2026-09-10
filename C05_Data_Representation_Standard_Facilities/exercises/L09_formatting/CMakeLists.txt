cmake_minimum_required(VERSION 3.28)
project(L09_formatting LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c05_add_observation(L09_formatting_observation observation.cpp)

add_executable(L09_formatting_bad_truncation validation/bad_truncation.cpp)
c05_configure_target(L09_formatting_bad_truncation)
c05_add_test(L09_formatting_bad_truncation_rejected COMMAND ${CMAKE_COMMAND}
    "-DEXECUTABLE=$<TARGET_FILE:L09_formatting_bad_truncation>"
    "-DEXPECTED_TEXT=format_to_n reports truncation"
    -P "${CMAKE_CURRENT_LIST_DIR}/../cmake/expect_failure.cmake" LABELS negative)
