cmake_minimum_required(VERSION 3.28)
project(P2_mini_execution LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME P2_mini_execution
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "mini pipeline consumes fresh input")
