cmake_minimum_required(VERSION 3.28)
project(C11_L02_connections LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c11_exercise(C11_L02 "partial send retains unsent suffix"
  HEADERS "${C11_EXERCISES}/include/c11/write_queue.hpp")
if(C11_BUILD_REFERENCE)
  add_executable(C11_L02_bad_capacity checks.cpp)
  c11_target(C11_L02_bad_capacity)
  target_include_directories(C11_L02_bad_capacity PRIVATE bad_capacity)
  target_sources(C11_L02_bad_capacity PRIVATE bad_capacity/solution.hpp)
  c11_test(C11_L02_bad_capacity_rejected C11_L02_bad_capacity EXIT 1
    DIAGNOSTIC "queue owns compact payload independently of input capacity" LABEL checker)
  add_executable(C11_L02_bad_lifo checks.cpp)
  c11_target(C11_L02_bad_lifo)
  target_include_directories(C11_L02_bad_lifo PRIVATE bad_lifo)
  target_sources(C11_L02_bad_lifo PRIVATE bad_lifo/solution.hpp)
  c11_test(C11_L02_bad_lifo_rejected C11_L02_bad_lifo EXIT 1
    DIAGNOSTIC "multiple messages preserve FIFO through partial writes" LABEL checker)
  add_executable(C11_L02_baseline baseline.cpp)
  c11_target(C11_L02_baseline)
  target_sources(C11_L02_baseline PRIVATE
    "${C11_EXERCISES}/include/c11/frame_contract.hpp"
    "${C11_EXERCISES}/include/c11/socket.hpp")
  c11_test(C11_L02_baseline C11_L02_baseline LABEL reference)
  add_executable(C11_L02_states states.cpp)
  c11_target(C11_L02_states)
  target_sources(C11_L02_states PRIVATE
    "${C11_EXERCISES}/include/c11/connection.hpp"
    "${C11_EXERCISES}/include/c11/frame.hpp"
    "${C11_EXERCISES}/include/c11/frame_contract.hpp"
    "${C11_EXERCISES}/include/c11/write_queue.hpp")
  c11_test(C11_L02_states C11_L02_states LABEL reference)
  add_executable(C11_L02_loopback loopback.cpp)
  c11_target(C11_L02_loopback)
  target_sources(C11_L02_loopback PRIVATE
    "${C11_EXERCISES}/include/c11/connection.hpp"
    "${C11_EXERCISES}/include/c11/frame.hpp"
    "${C11_EXERCISES}/include/c11/frame_contract.hpp"
    "${C11_EXERCISES}/include/c11/socket.hpp"
    "${C11_EXERCISES}/include/c11/write_queue.hpp")
  c11_test(C11_L02_loopback C11_L02_loopback LABEL observation)
endif()
learncpp_setup_ide("C11_L02_student")
