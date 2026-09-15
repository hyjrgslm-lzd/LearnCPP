cmake_minimum_required(VERSION 3.28)
project(G3_sync_wait_impl LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(G3_sync_wait_impl REFERENCE_SOURCES solution.cpp)

learncpp_setup_ide("G3_sync_wait_impl")
