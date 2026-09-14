cmake_minimum_required(VERSION 3.24)
project(C18_L07_python_buffers LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)
include(../backends/PythonSupport.cmake)

function(c18_l07_variant variant implementation)
  if(NOT C18_ENABLE_PYTHON)
    return()
  endif()
  c18_python_extension(c18_l07_${variant} "${implementation}/solution.cpp")
  target_compile_definitions(c18_l07_${variant} PRIVATE C18_MODULE_NAME=c18_l07_${variant})
endfunction()

c18_l07_variant(student student)
if(C18_BUILD_REFERENCE)
  c18_l07_variant(reference reference)
  c18_l07_variant(good validation/good)
  c18_l07_variant(bad validation/bad)
endif()

if(C18_ENABLE_PYTHON AND C18_BUILD_REFERENCE)
  c18_add_command_test(c18_l07_reference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l07_reference>"
    LABELS "reference;python;buffer")
  c18_add_command_test(c18_l07_good
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l07_good>"
    LABELS "good;python;buffer")
  c18_add_command_test(c18_l07_bad_rejected
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l07_bad>"
    EXPECT_FAILURE "check failed: Py_buffer was not released" LABELS "checker;python;buffer")
endif()
if(C18_ENABLE_PYTHON AND C18_TEST_STUDENTS)
  c18_add_command_test(c18_l07_student
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l07_student>"
    LABELS "student;python;buffer")
endif()
