cmake_minimum_required(VERSION 3.28)
project(P1_future_basics LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(P1_future_basics
    STANDARD 20
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
)

learncpp_setup_ide("P1_future_basics")
