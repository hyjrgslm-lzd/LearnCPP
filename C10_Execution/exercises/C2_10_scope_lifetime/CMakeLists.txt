cmake_minimum_required(VERSION 3.28)
project(C2_10_scope_lifetime LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME C2_10_scope_lifetime
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "scope.on_empty drains accepted work")
