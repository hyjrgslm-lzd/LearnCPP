cmake_minimum_required(VERSION 3.28)
project(C11_L10_service_policy LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c11_exercise(C11_L10 "non-idempotent outcomes are not retried"
  HEADERS "${C11_EXERCISES}/include/c11/service_policy.hpp")
if(C11_BUILD_REFERENCE)
  add_executable(C11_L10_pool pool.cpp)
  c11_target(C11_L10_pool)
  target_sources(C11_L10_pool PRIVATE
    "${C11_EXERCISES}/include/c11/connection.hpp"
    "${C11_EXERCISES}/include/c11/frame.hpp"
    "${C11_EXERCISES}/include/c11/frame_contract.hpp"
    "${C11_EXERCISES}/include/c11/reactor.hpp"
    "${C11_EXERCISES}/include/c11/socket.hpp"
    "${C11_EXERCISES}/include/c11/socket_pool.hpp"
    "${C11_EXERCISES}/include/c11/write_queue.hpp")
  c11_test(C11_L10_pool C11_L10_pool LABEL reference TIMEOUT 15)
endif()
learncpp_setup_ide("C11_L10_student")
