cmake_minimum_required(VERSION 3.28)
project(values_blind_a01_r2 LANGUAGES CXX)

add_executable(a01_boundary_positive a01_boundary_positive.cpp)
target_compile_features(a01_boundary_positive PRIVATE cxx_std_23)
set_target_properties(a01_boundary_positive PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_include_directories(a01_boundary_positive PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../exercises/A01_compiletime_values/src/reference"
)

if(MSVC)
    target_compile_options(a01_boundary_positive PRIVATE /utf-8 /EHsc /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(a01_boundary_positive PRIVATE -Wall -Wextra -Wpedantic)
endif()
