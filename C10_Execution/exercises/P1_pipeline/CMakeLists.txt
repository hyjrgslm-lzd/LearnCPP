cmake_minimum_required(VERSION 3.28)
project(P1_pipeline LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StudySetup)
include_directories("${C10_REPO}/C07_OS_Memory_System_IO/exercises/include")

set(extra_libs)
if(TARGET c10_liburing)
  list(APPEND extra_libs c10_liburing)
elseif(NOT WIN32)
  message(FATAL_ERROR "P1_pipeline requires -DC10_STUDY_ENABLE_IO_URING=ON and c10_liburing on Linux")
endif()

c10_add_exercise(
  NAME P1_pipeline
  HEADER solution.hpp
  CHECK main.cpp
  BAD_DIAGNOSTIC "invalid records recovered"
  LIBRARIES ${extra_libs}
  ALLOW_SKIP)

foreach(variant student reference validation_good validation_bad)
  if(TARGET P1_pipeline_${variant})
    target_sources(P1_pipeline_${variant} PRIVATE
      "${C10_REPO}/C07_OS_Memory_System_IO/exercises/include/c07/os.hpp"
      "${C10_EXERCISES}/include/c10/native_io.hpp")
  endif()
endforeach()
