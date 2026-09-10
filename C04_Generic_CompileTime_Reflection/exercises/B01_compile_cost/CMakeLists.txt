cmake_minimum_required(VERSION 3.28)
project(B01_compile_cost LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_observation(B01_compile_cost_contracts observations/contracts.cpp)
