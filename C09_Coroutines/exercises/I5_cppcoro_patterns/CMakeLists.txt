cmake_minimum_required(VERSION 3.25)
project(I5_cppcoro_patterns LANGUAGES CXX)

if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_require_option(COROUTINE_STUDY_ENABLE_CPPCORO "I5_cppcoro_patterns")

coroutine_study_add_exercise(
    I5_cppcoro_patterns
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    LIBRARIES cppcoro::cppcoro
    STUDENT_TEST
)

learncpp_setup_ide("I5_cppcoro_patterns")
