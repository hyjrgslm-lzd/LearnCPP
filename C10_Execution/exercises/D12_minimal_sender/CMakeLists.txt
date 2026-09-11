cmake_minimum_required(VERSION 3.28)
project(D12_minimal_sender LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME D12_minimal_sender
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "error mode sends set_error")
