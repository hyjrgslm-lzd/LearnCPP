cmake_minimum_required(VERSION 3.28)
project(L06_transcoding LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c05_add_exercise(
    NAME L06_transcoding
    HEADER utf_transcode.hpp
    CHECK checks/transcode_checks.cpp
    BAD_DIAGNOSTIC "rejects overlong slash")
