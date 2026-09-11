cmake_minimum_required(VERSION 3.28)
project(C1_8_cancellation LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME C1_8_cancellation
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "cancel during work completes stopped")
