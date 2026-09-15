cmake_minimum_required(VERSION 3.28)
project(D3_final_suspend_symmetric LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(D3_final_suspend_symmetric STUDENT_TEST REFERENCE_SOURCES solution.cpp)

learncpp_setup_ide("D3_final_suspend_symmetric")
