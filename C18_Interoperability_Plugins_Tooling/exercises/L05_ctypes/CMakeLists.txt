cmake_minimum_required(VERSION 3.24)
project(C18_L05_ctypes LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)

add_library(c18_l05_instrumented_plugin SHARED fixtures/instrumented_plugin.c)
c18_target(c18_l05_instrumented_plugin)
target_compile_definitions(c18_l05_instrumented_plugin PRIVATE C18_PLUGIN_BUILD)
set_target_properties(c18_l05_instrumented_plugin PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

foreach(variant student reference good bad)
  if(variant STREQUAL "reference" OR variant STREQUAL "good" OR variant STREQUAL "bad")
    if(NOT C18_BUILD_REFERENCE)
      continue()
    endif()
  endif()
  set(src "${CMAKE_CURRENT_SOURCE_DIR}/${variant}/solution.py")
  if(variant STREQUAL "good")
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/validation/good/solution.py")
  elseif(variant STREQUAL "bad")
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad/solution.py")
  endif()
  if(variant STREQUAL "bad")
    c18_add_command_test(c18_l05_bad_rejected
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "${src}" "$<TARGET_FILE:c18_l05_instrumented_plugin>"
      EXPECT_FAILURE "check failed: callback target was not kept alive" LABELS "checker;ctypes")
  elseif(variant STREQUAL "student")
    if(C18_TEST_STUDENTS)
      c18_add_command_test(c18_l05_student
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "${src}" "$<TARGET_FILE:c18_l05_instrumented_plugin>"
        LABELS "student;ctypes")
    endif()
  else()
    c18_add_command_test(c18_l05_${variant}
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "${src}" "$<TARGET_FILE:c18_l05_instrumented_plugin>"
      LABELS "${variant};ctypes")
  endif()
endforeach()
