cmake_minimum_required(VERSION 3.25)
project(I4_cobalt_channel LANGUAGES CXX)

if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_require_option(COROUTINE_STUDY_ENABLE_COBALT "I4_cobalt_channel")

coroutine_study_add_exercise(
    I4_cobalt_channel
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    LIBRARIES Boost::cobalt
    STUDENT_TEST
)

learncpp_setup_ide("I4_cobalt_channel")
