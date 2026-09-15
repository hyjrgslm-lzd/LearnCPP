cmake_minimum_required(VERSION 3.28)
project(D1_promise_8_hooks LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(D1_promise_8_hooks STUDENT_TEST REFERENCE_SOURCES solution.cpp)

learncpp_setup_ide("D1_promise_8_hooks")
