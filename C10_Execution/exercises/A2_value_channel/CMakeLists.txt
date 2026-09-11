cmake_minimum_required(VERSION 3.28)
project(A2_value_channel LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
c10_add_exercise(
  NAME A2_value_channel
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "sender body stays lazy until sync_wait")
