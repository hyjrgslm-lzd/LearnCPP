cmake_minimum_required(VERSION 3.28)
project(C1_2_split_evolution LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(C1_2_split_evolution main.cpp)
