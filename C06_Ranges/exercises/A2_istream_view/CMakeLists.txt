cmake_minimum_required(VERSION 3.28)
project(A2_istream_view LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(A2_istream_view main.cpp)
