cmake_minimum_required(VERSION 3.28)
project(A1_first_generator LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(A1_first_generator
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
)

learncpp_setup_ide("A1_first_generator")
