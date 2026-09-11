cmake_minimum_required(VERSION 3.28)
project(D14_coroutine_bridge LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME D14_coroutine_bridge
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "sender graph consumes fresh input")
