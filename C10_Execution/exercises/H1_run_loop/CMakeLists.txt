cmake_minimum_required(VERSION 3.28)
project(H1_run_loop LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME H1_run_loop
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "completion runs on loop thread")
