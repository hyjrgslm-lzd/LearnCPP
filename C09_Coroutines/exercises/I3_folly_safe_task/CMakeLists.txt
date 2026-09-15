cmake_minimum_required(VERSION 3.25)
project(I3_folly_safe_task LANGUAGES CXX)

if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_require_option(COROUTINE_STUDY_ENABLE_FOLLY "I3_folly_safe_task")

coroutine_study_add_exercise(
    I3_folly_safe_task
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    LIBRARIES folly::folly
    STUDENT_TEST
)

learncpp_setup_ide("I3_folly_safe_task")
