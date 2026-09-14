cmake_minimum_required(VERSION 3.24)
project(c16_l08 LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c16_exercise(c16_l08 QT Core Gui Widgets Test EXPECT "check failed:")
