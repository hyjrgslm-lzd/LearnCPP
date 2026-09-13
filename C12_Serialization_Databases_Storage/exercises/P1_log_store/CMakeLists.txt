cmake_minimum_required(VERSION 3.25)
project(C12_P1_log_store LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c12_exercise(C12_P1_apply "atomic rejection")
if(C12_BUILD_REFERENCE)
  add_executable(C12_P1_store main.cpp)
  c12_target(C12_P1_store)
  c12_test(C12_P1_store LABELS reference)
  c12_test(C12_P1_recovery COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/recovery.py"
    "$<TARGET_FILE:C12_P1_store>" TIMEOUT 120 LABELS recovery)
endif()

if(C12_BUILD_REFERENCE)
  c12_observation(C12_P1_replay_cost replay_cost.cpp)
endif()

if(C12_BUILD_REFERENCE)
  add_executable(C12_P1_boundaries boundaries.cpp)
  c12_target(C12_P1_boundaries)
  c12_test(C12_P1_boundaries TIMEOUT 120 LABELS boundary)
  c12_test(C12_P1_extensions COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/extensions.py"
    "$<TARGET_FILE:C12_P1_store>" TIMEOUT 60 LABELS reference)
  c12_test(C12_P1_checkpoint_cost COMMAND "$<TARGET_FILE:C12_P1_replay_cost>" checkpoint LABELS observation)
endif()
