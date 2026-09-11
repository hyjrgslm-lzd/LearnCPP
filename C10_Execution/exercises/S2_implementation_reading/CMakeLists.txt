cmake_minimum_required(VERSION 3.28)
project(S2_implementation_reading LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_observation(S2_implementation_reading main.cpp LABELS source_reading)
