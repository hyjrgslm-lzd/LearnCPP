cmake_minimum_required(VERSION 3.28)
project(Capstone1_async_crawler LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(Capstone1_async_crawler
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    STUDENT_TEST
)
target_include_directories(Capstone1_async_crawler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET Capstone1_async_crawler_reference)
    target_include_directories(Capstone1_async_crawler_reference PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

learncpp_setup_ide("Capstone1_async_crawler")
