cmake_minimum_required(VERSION 3.24)
project(C18_L02 LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)
c18_exercise(L02_buffers EXPECT "check failed: failure leaves output unchanged")
