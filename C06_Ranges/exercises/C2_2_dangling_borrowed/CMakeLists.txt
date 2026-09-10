cmake_minimum_required(VERSION 3.28)
project(C2_2_dangling_borrowed LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(C2_2_dangling_borrowed main.cpp)
