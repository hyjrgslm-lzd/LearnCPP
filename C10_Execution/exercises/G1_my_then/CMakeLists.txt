cmake_minimum_required(VERSION 3.28)
project(G1_my_then LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME G1_my_then
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "error channel forwarded as set_error")
