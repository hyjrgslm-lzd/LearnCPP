cmake_minimum_required(VERSION 3.28)
project(C1_stop_token_cancel LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(C1_stop_token_cancel
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    STUDENT_TEST
)

learncpp_setup_ide("C1_stop_token_cancel")
