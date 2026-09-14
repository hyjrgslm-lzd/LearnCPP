cmake_minimum_required(VERSION 3.24)
project(C18_L10_pybind11 LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)

if(NOT C18_ENABLE_PYBIND11)
  message(STATUS "C18 L10 pybind11 disabled; configure with -DC18_ENABLE_PYBIND11=ON and pybind11 3.0.4 to build it")
  return()
endif()

find_package(pybind11 3.0.4 EXACT REQUIRED CONFIG)

function(c18_l10_module variant source)
  pybind11_add_module(c18_l10_${variant} "${source}")
endfunction()

function(c18_l10_check variant labels)
  c18_add_command_test(c18_l10_${variant}
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l10_${variant}>"
    LABELS "${labels};pybind11")
endfunction()

c18_l10_module(student student/solution.cpp)

if(C18_BUILD_REFERENCE)
  c18_l10_module(reference reference/solution.cpp)
  c18_l10_check(reference reference)
  c18_l10_module(good validation/good/solution.cpp)
  c18_l10_check(good good)
  c18_l10_module(bad validation/bad/solution.cpp)
  c18_add_command_test(c18_l10_bad_rejected
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l10_bad>"
    EXPECT_FAILURE "check failed: keep_alive did not keep token alive"
    LABELS "checker;pybind11")

  add_executable(c18_l10_embed embedding.cpp)
  c18_target(c18_l10_embed)
  target_link_libraries(c18_l10_embed PRIVATE pybind11::embed)
  c18_add_test(c18_l10_embed c18_l10_embed LABELS "reference;pybind11;embedding")
endif()

if(C18_TEST_STUDENTS)
  c18_l10_check(student student)
endif()
