cmake_minimum_required(VERSION 3.28)
project(C3_async_scope LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(C3_async_scope
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    STUDENT_TEST
)

learncpp_setup_ide("C3_async_scope")
