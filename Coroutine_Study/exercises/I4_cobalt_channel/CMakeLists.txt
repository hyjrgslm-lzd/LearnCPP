cmake_minimum_required(VERSION 3.25)
project(I4_cobalt_channel LANGUAGES CXX)

coroutine_study_add_exercise(
    I4_cobalt_channel
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    LIBRARIES Boost::cobalt
)
