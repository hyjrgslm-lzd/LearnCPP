cmake_minimum_required(VERSION 3.28)
project(E3_evolution_compare LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
c10_add_exercise(NAME E3_evolution_compare HEADER solution.hpp CHECK main.cpp
                 BAD_DIAGNOSTIC "member dispatch has first priority")
