cmake_minimum_required(VERSION 3.28)
project(E3_cpo_tagdispatch_compare LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_observation(E3_cpo_tagdispatch_compare main.cpp)
