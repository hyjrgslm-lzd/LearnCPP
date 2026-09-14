cmake_minimum_required(VERSION 3.24)
project(c16_l12 LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c16_exercise(c16_l12 QT Core Gui Multimedia Test EXPECT "check failed:")
