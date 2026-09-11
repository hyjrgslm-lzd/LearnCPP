cmake_minimum_required(VERSION 3.28)
project(R1_task_scope LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_observation(R1_task_scope main.cpp)
