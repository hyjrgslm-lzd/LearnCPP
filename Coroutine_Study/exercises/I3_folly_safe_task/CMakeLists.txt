cmake_minimum_required(VERSION 3.25)
project(I3_folly_safe_task LANGUAGES CXX)

coroutine_study_add_exercise(
    I3_folly_safe_task
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    LIBRARIES folly::folly
)
