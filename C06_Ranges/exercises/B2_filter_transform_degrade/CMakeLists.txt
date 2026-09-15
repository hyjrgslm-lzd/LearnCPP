cmake_minimum_required(VERSION 3.28)
project(B2_filter_transform_degrade LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(B2_filter_transform_degrade main.cpp)


learncpp_setup_ide(B2_filter_transform_degrade)
