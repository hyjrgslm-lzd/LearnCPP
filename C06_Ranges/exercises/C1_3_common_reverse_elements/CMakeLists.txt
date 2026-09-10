cmake_minimum_required(VERSION 3.28)
project(C1_3_common_reverse_elements LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(C1_3_common_reverse_elements main.cpp)
