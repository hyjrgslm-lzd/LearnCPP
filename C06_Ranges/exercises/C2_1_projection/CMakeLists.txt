cmake_minimum_required(VERSION 3.28)
project(C2_1_projection LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(C2_1_projection main.cpp)
