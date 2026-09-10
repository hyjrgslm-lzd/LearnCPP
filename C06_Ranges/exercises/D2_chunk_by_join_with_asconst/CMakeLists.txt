cmake_minimum_required(VERSION 3.28)
project(D2_chunk_by_join_with_asconst LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)
ranges_add_observation(D2_chunk_by_join_with_asconst main.cpp)
