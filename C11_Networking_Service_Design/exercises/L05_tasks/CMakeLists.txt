cmake_minimum_required(VERSION 3.28)
project(C11_L05_tasks LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c11_exercise(C11_L05 "cancellation wins before worker acknowledgement"
  HEADERS
    "${C11_EXERCISES}/include/c11/task_registry.hpp"
    "${C11_EXERCISES}/include/c11/task_types.hpp")
if(TARGET C11_L05_reference)
  target_sources(C11_L05_reference PRIVATE "${C11_EXERCISES}/include/c11/task_policy.hpp")
endif()
if(TARGET C11_L05_bad)
  target_sources(C11_L05_bad PRIVATE "${C11_EXERCISES}/include/c11/task_policy.hpp")
endif()
