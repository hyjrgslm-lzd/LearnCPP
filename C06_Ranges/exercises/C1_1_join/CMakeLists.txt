cmake_minimum_required(VERSION 3.28)
project(C1_1_join LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(C1_1_join main.cpp)


learncpp_setup_ide(C1_1_join)
