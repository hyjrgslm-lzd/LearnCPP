cmake_minimum_required(VERSION 3.28)
project(E1_co_await_lookup LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(E1_co_await_lookup REFERENCE_SOURCES solution.cpp)

learncpp_setup_ide("E1_co_await_lookup")
