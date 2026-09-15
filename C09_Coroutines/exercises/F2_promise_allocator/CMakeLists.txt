cmake_minimum_required(VERSION 3.28)
project(F2_promise_allocator LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(F2_promise_allocator REFERENCE_SOURCES solution.cpp)

learncpp_setup_ide("F2_promise_allocator")
