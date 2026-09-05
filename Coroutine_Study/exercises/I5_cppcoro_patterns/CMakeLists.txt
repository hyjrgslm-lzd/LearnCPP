cmake_minimum_required(VERSION 3.25)
project(I5_cppcoro_patterns LANGUAGES CXX)

coroutine_study_add_exercise(
    I5_cppcoro_patterns
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    LIBRARIES cppcoro::cppcoro
)
