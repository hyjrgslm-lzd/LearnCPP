cmake_minimum_required(VERSION 3.28)
project(L02_sequence_containers LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_observation(L02_sequence_containers main.cpp)
