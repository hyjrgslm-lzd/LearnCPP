cmake_minimum_required(VERSION 3.28)
project(C11_L01_framing LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c11_exercise(C11_L01 "oversized length rejected"
  HEADERS "${C11_EXERCISES}/include/c11/frame_contract.hpp")
if(C11_BUILD_REFERENCE)
  if(TARGET C11_L01_reference)
    target_sources(C11_L01_reference PRIVATE "${C11_EXERCISES}/include/c11/frame.hpp")
  endif()
  add_executable(C11_L01_bad_high_digits checks.cpp)
  c11_target(C11_L01_bad_high_digits)
  target_include_directories(C11_L01_bad_high_digits PRIVATE bad_high_digits)
  target_sources(C11_L01_bad_high_digits PRIVATE
    bad_high_digits/solution.hpp
    "${C11_EXERCISES}/include/c11/frame_contract.hpp")
  c11_test(C11_L01_bad_high_digits_rejected C11_L01_bad_high_digits EXIT 1
    DIAGNOSTIC "high-order length digits cannot be discarded" LABEL checker)
endif()
learncpp_setup_ide("C11_L01_student")
