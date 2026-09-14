cmake_minimum_required(VERSION 3.24)
project(c16_l05 LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c16_exercise(c16_l05 QT Core Test EXPECT "check failed:")

add_executable(c16_l05_baseline baseline.cpp)
c16_target(c16_l05_baseline Core Test)
c16_add_test(c16_l05_baseline c16_l05_baseline LABELS observation)
