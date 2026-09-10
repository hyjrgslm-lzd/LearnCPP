cmake_minimum_required(VERSION 3.28)
project(B1_all_ref_owning LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(B1_all_ref_owning main.cpp)
