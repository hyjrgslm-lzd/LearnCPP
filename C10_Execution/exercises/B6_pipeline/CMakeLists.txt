cmake_minimum_required(VERSION 3.28)
project(B6_pipeline LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
c10_add_exercise(
  NAME B6_pipeline
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "parse and enrich use distinct execution resources")
