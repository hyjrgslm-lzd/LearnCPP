cmake_minimum_required(VERSION 3.28)
project(D13_sender_adaptor LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME D13_sender_adaptor
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "tap forwards stopped channel")
