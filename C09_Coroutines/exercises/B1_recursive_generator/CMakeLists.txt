cmake_minimum_required(VERSION 3.28)
project(B1_recursive_generator LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(B1_recursive_generator
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    STUDENT_TEST
)

learncpp_setup_ide("B1_recursive_generator")
