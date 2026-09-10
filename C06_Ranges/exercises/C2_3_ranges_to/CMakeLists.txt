cmake_minimum_required(VERSION 3.28)
project(C2_3_ranges_to LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(C2_3_ranges_to main.cpp)
