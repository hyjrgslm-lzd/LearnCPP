cmake_minimum_required(VERSION 3.28)
project(A3_co_await_future LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

find_package(Threads REQUIRED)
coroutine_study_add_exercise(A3_co_await_future
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    STUDENT_TEST
    LIBRARIES Threads::Threads
)

learncpp_setup_ide("A3_co_await_future")
