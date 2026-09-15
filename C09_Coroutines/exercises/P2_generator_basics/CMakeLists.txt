cmake_minimum_required(VERSION 3.28)
project(P2_generator_basics LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(P2_generator_basics
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
)

learncpp_setup_ide("P2_generator_basics")
