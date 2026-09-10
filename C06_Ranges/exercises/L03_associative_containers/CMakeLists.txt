cmake_minimum_required(VERSION 3.28)
project(L03_associative_containers LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_observation(L03_associative_containers main.cpp)
