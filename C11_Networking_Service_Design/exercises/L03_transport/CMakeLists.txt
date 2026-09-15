cmake_minimum_required(VERSION 3.28)
project(C11_L03_transport LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c11_exercise(C11_L03 "compression cycles rejected"
  HEADERS "${C11_EXERCISES}/include/c11/dns.hpp")
if(C11_BUILD_REFERENCE)
  foreach(defect nested length)
    add_executable(C11_L03_bad_${defect} checks.cpp)
    c11_target(C11_L03_bad_${defect})
    target_include_directories(C11_L03_bad_${defect} PRIVATE bad_${defect})
    target_sources(C11_L03_bad_${defect} PRIVATE
      bad_${defect}/solution.hpp
      bad_decoder.hpp
      "${C11_EXERCISES}/include/c11/dns.hpp")
    if(defect STREQUAL "nested")
      set(diagnostic "nested pointers preserve original consumed position")
    else()
      set(diagnostic "overlong complete hostname rejected")
    endif()
    c11_test(C11_L03_bad_${defect}_rejected C11_L03_bad_${defect} EXIT 1 DIAGNOSTIC "${diagnostic}" LABEL checker)
  endforeach()
  add_executable(C11_L03_sockets sockets.cpp)
  c11_target(C11_L03_sockets)
  target_sources(C11_L03_sockets PRIVATE
    "${C11_EXERCISES}/include/c11/dns.hpp"
    "${C11_EXERCISES}/include/c11/socket.hpp")
  c11_test(C11_L03_sockets C11_L03_sockets LABEL observation)
endif()
if(TARGET C11_L03_bad)
  target_sources(C11_L03_bad PRIVATE bad_decoder.hpp)
endif()
learncpp_setup_ide("C11_L03_student")
