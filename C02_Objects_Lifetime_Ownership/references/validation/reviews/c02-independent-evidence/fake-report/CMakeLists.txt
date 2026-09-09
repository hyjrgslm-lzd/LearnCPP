cmake_minimum_required(VERSION 3.28)

project(l04_fake_report LANGUAGES CXX)

add_executable(l04_fake_report
    ../../../../../exercises/L04_construction/checks/run_checks.cpp
    construction_lab.cpp
)

target_compile_features(l04_fake_report PRIVATE cxx_std_23)
set_target_properties(l04_fake_report PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_include_directories(l04_fake_report PRIVATE
    .
    ../../../../../exercises/L04_construction/checks
    ../../../../../../Engineering_Study/exercises/include
)
target_compile_definitions(l04_fake_report PRIVATE
    L04_IMPLEMENTATION_HEADER="construction_lab.hpp"
)
