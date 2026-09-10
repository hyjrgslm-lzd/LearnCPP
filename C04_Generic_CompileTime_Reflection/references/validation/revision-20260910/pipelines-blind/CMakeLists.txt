cmake_minimum_required(VERSION 3.25)
project(c04_pipelines_blind LANGUAGES CXX)

add_executable(a05_blind_expression_core a05_blind_expression_core.cpp)
target_compile_features(a05_blind_expression_core PRIVATE cxx_std_23)

add_executable(a04_probe_good a04_probe.cpp)
target_compile_features(a04_probe_good PRIVATE cxx_std_23)
target_include_directories(a04_probe_good PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../../../exercises/A04_type_pipelines/validation/good")

add_executable(a05_noexcept_probe_good a05_noexcept_probe.cpp)
target_compile_features(a05_noexcept_probe_good PRIVATE cxx_std_23)
target_include_directories(a05_noexcept_probe_good PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../../../exercises/A05_expression_templates/validation/good")

if(MSVC)
  target_compile_options(a05_blind_expression_core PRIVATE /W4 /permissive-)
  target_compile_options(a04_probe_good PRIVATE /W4 /permissive-)
  target_compile_options(a05_noexcept_probe_good PRIVATE /W4 /permissive-)
else()
  target_compile_options(a05_blind_expression_core PRIVATE -Wall -Wextra -pedantic)
  target_compile_options(a04_probe_good PRIVATE -Wall -Wextra -pedantic)
  target_compile_options(a05_noexcept_probe_good PRIVATE -Wall -Wextra -pedantic)
endif()
