cmake_minimum_required(VERSION 3.28)
project(A3_repeat_cartesian LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(A3_repeat_cartesian main.cpp)
