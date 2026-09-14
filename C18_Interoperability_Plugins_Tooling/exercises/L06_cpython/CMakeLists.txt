cmake_minimum_required(VERSION 3.24)
project(C18_L06_cpython LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)
include(../backends/PythonSupport.cmake)

function(c18_l06_variant variant implementation)
  if(NOT C18_ENABLE_PYTHON)
    return()
  endif()
  c18_python_extension(c18_l06_${variant} "${implementation}/solution.cpp")
  target_compile_definitions(c18_l06_${variant} PRIVATE C18_MODULE_NAME=c18_l06_${variant})
endfunction()

c18_l06_variant(student student)
if(C18_BUILD_REFERENCE)
  c18_l06_variant(reference reference)
  c18_l06_variant(good validation/good)
  c18_l06_variant(bad validation/bad)
endif()

if(C18_ENABLE_PYTHON AND C18_BUILD_REFERENCE)
  c18_add_command_test(c18_l06_reference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l06_reference>"
    LABELS "reference;python;cpython")
  c18_add_command_test(c18_l06_good
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l06_good>"
    LABELS "good;python;cpython")
  c18_add_command_test(c18_l06_bad_rejected
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l06_bad>"
    EXPECT_FAILURE "check failed: error indicator leaked through a non-null return" LABELS "checker;python;cpython")
endif()
if(C18_ENABLE_PYTHON AND C18_TEST_STUDENTS)
  c18_add_command_test(c18_l06_student
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l06_student>"
    LABELS "student;python;cpython")
endif()
