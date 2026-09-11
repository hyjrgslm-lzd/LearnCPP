cmake_minimum_required(VERSION 3.28)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(C10_B02_bulk_domain LANGUAGES CXX)
    include("../cmake/StudySetup.cmake")
endif()

c10_add_observation(B02_bulk_domain main.cpp LABELS domain bulk)
