cmake_minimum_required(VERSION 3.28)
project(L07_text_semantics LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c05_add_observation(L07_text_semantics_observation observation.cpp)

add_executable(L07_text_semantics_bad_identity validation/bad_identity.cpp)
c05_configure_target(L07_text_semantics_bad_identity)
c05_add_test(L07_text_semantics_bad_identity_rejected COMMAND ${CMAKE_COMMAND}
    "-DEXECUTABLE=$<TARGET_FILE:L07_text_semantics_bad_identity>"
    "-DEXPECTED_TEXT=canonical-equivalence is not byte identity"
    -P "${CMAKE_CURRENT_LIST_DIR}/../cmake/expect_failure.cmake" LABELS negative)
