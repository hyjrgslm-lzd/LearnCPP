cmake_minimum_required(VERSION 3.28)
project(values_blind_a01 LANGUAGES CXX)

add_executable(values_blind_a01
    ../../../../../exercises/A01_compiletime_values/checks/compiletime_values_checks.cpp
)

target_compile_features(values_blind_a01 PRIVATE cxx_std_23)
set_target_properties(values_blind_a01 PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_include_directories(values_blind_a01 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../exercises/A01_compiletime_values/checks"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../C01_Build_Compile_Link/exercises/include"
)

if(MSVC)
    target_compile_options(values_blind_a01 PRIVATE /utf-8 /EHsc /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(values_blind_a01 PRIVATE -Wall -Wextra -Wpedantic)
endif()
