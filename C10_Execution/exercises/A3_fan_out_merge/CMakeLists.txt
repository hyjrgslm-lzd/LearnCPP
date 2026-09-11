cmake_minimum_required(VERSION 3.28)
project(A3_fan_out_merge LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
c10_add_exercise(
  NAME A3_fan_out_merge
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "fan-out branches do not run while graph is only being built")
