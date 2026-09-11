cmake_minimum_required(VERSION 3.28)
project(D11_minimal_receiver LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME D11_minimal_receiver
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "stopped channel recorded")
