cmake_minimum_required(VERSION 3.28)
project(F2_completion_signatures LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
c10_add_exercise(NAME F2_completion_signatures HEADER solution.hpp CHECK main.cpp
                 STUDENT_EXPECT_EXIT 1
                 STUDENT_DIAGNOSTIC "value channel signatures extracted"
                 BAD_DIAGNOSTIC "void value transform emits set_value with no arguments")
