cmake_minimum_required(VERSION 3.28)
project(E3_trivial_awaitable LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(E3_trivial_awaitable REFERENCE_SOURCES solution.cpp)

learncpp_setup_ide("E3_trivial_awaitable")
