cmake_minimum_required(VERSION 3.28)
project(L06_process_ipc LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c07_add_exercise(NAME L06_process_ipc HEADER process_ipc.hpp CHECK checks/process_ipc_checks.cpp
    BAD_DIAGNOSTIC "child witness proves spawned fixture")
c07_add_observation(L06_file_locking checks/file_locking.cpp LABELS file_locking)
