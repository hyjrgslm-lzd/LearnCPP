cmake_minimum_required(VERSION 3.28)
project(c05_spdlog_pollution_probe LANGUAGES CXX)

set(DATA_STUDY_ENABLE_FORMAT_LIBS ON CACHE BOOL "" FORCE)
set(DATA_STUDY_SPDLOG_BACKEND fmt CACHE STRING "" FORCE)

add_library(spdlog_external STATIC external.cpp)
add_library(spdlog::spdlog ALIAS spdlog_external)

include("${CMAKE_CURRENT_LIST_DIR}/../../../../exercises/cmake/FormatLibraries.cmake")

add_executable(app main.cpp)
c05_link_spdlog(app)
