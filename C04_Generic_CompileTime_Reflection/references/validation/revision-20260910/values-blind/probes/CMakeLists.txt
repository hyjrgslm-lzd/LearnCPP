cmake_minimum_required(VERSION 3.28)
project(values_blind_probes LANGUAGES CXX)

add_executable(a01_boundary_probe a01_boundary_probe.cpp)
target_include_directories(a01_boundary_probe PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../exercises/A01_compiletime_values/src/reference"
)

add_executable(a03_extra_probe a03_extra_probe.cpp)
target_include_directories(a03_extra_probe PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../exercises/A03_explicit_object/src/reference"
)

foreach(target a01_boundary_probe a03_extra_probe)
    target_compile_features(${target} PRIVATE cxx_std_23)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /EHsc /W4 /permissive- /Zc:__cplusplus)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
