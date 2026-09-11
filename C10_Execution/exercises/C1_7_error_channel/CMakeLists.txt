cmake_minimum_required(VERSION 3.28)
project(C1_7_error_channel LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME C1_7_error_channel
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "let_error returns fallback sender")
