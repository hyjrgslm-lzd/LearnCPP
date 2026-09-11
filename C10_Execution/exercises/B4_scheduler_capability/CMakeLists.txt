cmake_minimum_required(VERSION 3.28)
project(B4_scheduler_capability LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
c10_add_exercise(
  NAME B4_scheduler_capability
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "all requested scheduled tasks complete")
