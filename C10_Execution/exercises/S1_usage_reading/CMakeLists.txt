cmake_minimum_required(VERSION 3.28)
project(S1_usage_reading LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_enable_parallel_scheduler()
c10_add_observation(S1_usage_reading main.cpp LABELS source_reading LIBRARIES c10_parallel_scheduler)
