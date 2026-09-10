cmake_minimum_required(VERSION 3.28)
project(D1_zip_adjacent_chunk LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(D1_zip_adjacent_chunk main.cpp)
