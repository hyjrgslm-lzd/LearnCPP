cmake_minimum_required(VERSION 3.25)
project(c03_callable_qualifier_probe LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../../../../../exercises/cmake/StudySetup.cmake")

add_executable(c03_callable_qualifier_probe callable_qualifier_probe.cpp)
c03_configure_target(c03_callable_qualifier_probe)
enable_testing()
add_test(NAME c03_callable_qualifier_probe COMMAND c03_callable_qualifier_probe)
