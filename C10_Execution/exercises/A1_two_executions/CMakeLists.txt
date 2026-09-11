cmake_minimum_required(VERSION 3.28)
project(A1_two_executions LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
c10_add_exercise(
  NAME A1_two_executions
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "four-way split covers each element exactly once")
