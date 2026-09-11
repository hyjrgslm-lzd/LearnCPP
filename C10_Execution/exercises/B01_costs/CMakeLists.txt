cmake_minimum_required(VERSION 3.28)
project(B01_costs LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c10_add_observation(B01_costs main.cpp ARGS --self-test LABELS measurement)
