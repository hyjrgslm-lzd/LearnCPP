cmake_minimum_required(VERSION 3.28)
project(E2_niebloid LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_observation(E2_niebloid main.cpp)
