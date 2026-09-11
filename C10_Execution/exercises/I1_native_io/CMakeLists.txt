cmake_minimum_required(VERSION 3.28)
project(I1_native_io LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
include_directories("${C10_REPO}/C07_OS_Memory_System_IO/exercises/include")

set(extra_libs)
if(TARGET c10_liburing)
  list(APPEND extra_libs c10_liburing)
elseif(NOT WIN32)
  message(FATAL_ERROR "I1_native_io requires -DC10_STUDY_ENABLE_IO_URING=ON and c10_liburing on Linux")
endif()

c10_add_exercise(
  NAME I1_native_io
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "offset read bytes match"
  LIBRARIES ${extra_libs}
  ALLOW_SKIP)

if(WIN32)
  foreach(variant student reference validation_good validation_bad)
    if(TARGET I1_native_io_${variant})
      target_compile_definitions(I1_native_io_${variant} PRIVATE C10_NATIVE_IO_TEST_POST_SUBMIT=1)
    endif()
  endforeach()
endif()
