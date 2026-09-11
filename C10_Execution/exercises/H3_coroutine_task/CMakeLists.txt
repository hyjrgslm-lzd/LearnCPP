cmake_minimum_required(VERSION 3.28)
project(H3_coroutine_task LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)

c10_add_exercise(
  NAME H3_coroutine_task
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "task awaits synchronous sender value")
