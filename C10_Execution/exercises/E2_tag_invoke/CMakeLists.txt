cmake_minimum_required(VERSION 3.28)
project(E2_tag_invoke LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
c10_add_exercise(NAME E2_tag_invoke HEADER solution.hpp CHECK main.cpp
                 BAD_DIAGNOSTIC "connect dispatches through tag_invoke")
