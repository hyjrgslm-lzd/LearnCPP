cmake_minimum_required(VERSION 3.28)
project(C2_when_all_when_any LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(C2_when_all_when_any
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    STUDENT_TEST
)

learncpp_setup_ide("C2_when_all_when_any")
