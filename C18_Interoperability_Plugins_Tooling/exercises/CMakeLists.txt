cmake_minimum_required(VERSION 3.24)
project(C18_Interoperability_Plugins_Tooling LANGUAGES C CXX)
include(cmake/StudySetup.cmake)
c18_add_command_test(c18_runner_classifier COMMAND "${Python3_EXECUTABLE}" "${C18_COURSE}/tools/check_runner.py" LABELS infrastructure)

# Each complete teaching unit is also a standalone CMake entry point.
file(GLOB units RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "L*" "P*" "B*" "F*")
foreach(unit IN LISTS units)
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${unit}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${unit}/CMakeLists.txt")
    add_subdirectory("${unit}")
  endif()
endforeach()
