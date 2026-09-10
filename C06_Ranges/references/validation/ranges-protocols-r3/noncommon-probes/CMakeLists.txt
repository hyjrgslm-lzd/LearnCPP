cmake_minimum_required(VERSION 3.28)
project(ranges_protocols_noncommon_probes LANGUAGES CXX)
include(CTest)

add_executable(g2_noncommon_probe g2_noncommon_probe.cpp)
add_executable(g3_noncommon_probe g3_noncommon_probe.cpp)
foreach(t IN ITEMS g2_noncommon_probe g3_noncommon_probe)
  set_target_properties(${t} PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
  target_include_directories(${t} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../../../../C01_Build_Compile_Link/exercises/include")
  if(MSVC)
    target_compile_options(${t} PRIVATE /utf-8 /EHsc /permissive- /Zc:__cplusplus)
    set(CMAKE_CXX26_STANDARD_COMPILE_OPTION "/std:c++latest")
    set(CMAKE_CXX26_EXTENSION_COMPILE_OPTION "/std:c++latest")
  endif()
  add_test(NAME ${t} COMMAND ${t})
endforeach()

target_include_directories(g2_noncommon_probe PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../../../exercises/G2_my_transform_closure/src/reference")
target_include_directories(g3_noncommon_probe PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../../../exercises/G3_my_enumerate_borrowed/src/reference")