cmake_minimum_required(VERSION 3.28)
project(G2_pipe_syntax LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME G2_pipe_syntax
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "adaptor composition preserves left-to-right order")
