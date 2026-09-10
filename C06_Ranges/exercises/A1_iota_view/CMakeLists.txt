cmake_minimum_required(VERSION 3.28)
project(A1_iota_view LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(A1_iota_view main.cpp)
