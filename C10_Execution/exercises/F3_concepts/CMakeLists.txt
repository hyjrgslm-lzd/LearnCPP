cmake_minimum_required(VERSION 3.28)
project(F3_concepts LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
c10_add_exercise(NAME F3_concepts HEADER solution.hpp CHECK main.cpp
                 STUDENT_EXPECT_EXIT 1
                 STUDENT_DIAGNOSTIC "valid sender satisfies syntax concept"
                 BAD_DIAGNOSTIC "receiver_of rejects semantic mismatch")
