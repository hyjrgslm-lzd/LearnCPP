cmake_minimum_required(VERSION 3.28)
project(D3_std_generator LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(D3_std_generator main.cpp)
