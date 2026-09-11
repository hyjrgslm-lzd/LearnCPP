cmake_minimum_required(VERSION 3.28)
project(G3_retry LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME G3_retry
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "recursive retry budget exceeded")
