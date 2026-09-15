cmake_minimum_required(VERSION 3.28)
project(E2_await_suspend_three LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(E2_await_suspend_three REFERENCE_SOURCES solution.cpp)

learncpp_setup_ide("E2_await_suspend_three")
