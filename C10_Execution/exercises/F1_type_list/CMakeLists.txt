cmake_minimum_required(VERSION 3.28)
project(F1_type_list LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
c10_add_exercise(NAME F1_type_list HEADER solution.hpp CHECK main.cpp
                 STUDENT_EXPECT_EXIT 1
                 STUDENT_DIAGNOSTIC "concat joins lists"
                 BAD_DIAGNOSTIC "unique preserves first occurrence order")
