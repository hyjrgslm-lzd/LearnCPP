cmake_minimum_required(VERSION 3.28)
project(D2_eager_vs_lazy LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(D2_eager_vs_lazy REFERENCE_SOURCES solution.cpp)

learncpp_setup_ide("D2_eager_vs_lazy")
