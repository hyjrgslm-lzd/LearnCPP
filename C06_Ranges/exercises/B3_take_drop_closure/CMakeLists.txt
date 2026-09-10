cmake_minimum_required(VERSION 3.28)
project(B3_take_drop_closure LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(B3_take_drop_closure main.cpp)
