cmake_minimum_required(VERSION 3.28)
project(C2_9_environment LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME C2_9_environment
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "follow-up uses queried scheduler")
