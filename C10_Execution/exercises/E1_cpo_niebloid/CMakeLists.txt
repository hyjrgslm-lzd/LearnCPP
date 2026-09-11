cmake_minimum_required(VERSION 3.28)
project(E1_cpo_niebloid LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME E1_cpo_niebloid
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "member customization has first priority")
