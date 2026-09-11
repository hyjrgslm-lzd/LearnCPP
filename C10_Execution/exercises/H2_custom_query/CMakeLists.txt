cmake_minimum_required(VERSION 3.28)
project(H2_custom_query LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME H2_custom_query
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "override takes priority over parent")
