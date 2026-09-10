cmake_minimum_required(VERSION 3.28)
project(L07_packs LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L07_packs
    HEADER pack_tools.hpp
    CHECK checks/pack_tools_checks.cpp
    BAD_DIAGNOSTIC "empty all fold identity must be true"
)

c04_add_observation(L07_packs_observation observations/packs_observation.cpp)
