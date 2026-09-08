cmake_minimum_required(VERSION 4.2)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
endif()

project(I1_import_std LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

if(NOT CMAKE_CXX_COMPILER_IMPORT_STD)
    message(FATAL_ERROR "CMAKE_CXX_COMPILER_IMPORT_STD is empty; import std was not enabled for this CMake/compiler pair")
endif()
if(NOT 23 IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
    message(FATAL_ERROR "CMAKE_CXX_COMPILER_IMPORT_STD does not include 23")
endif()

add_executable(I1_import_std_reference reference/main.cpp)
target_compile_features(I1_import_std_reference PRIVATE cxx_std_23)
set_property(TARGET I1_import_std_reference PROPERTY CXX_MODULE_STD ON)
set_target_properties(I1_import_std_reference PROPERTIES CXX_EXTENSIONS OFF)
set_property(TARGET I1_import_std_reference PROPERTY CXX_SCAN_FOR_MODULES ON)

engineering_add_test(I1_import_std_reference
    COMMAND I1_import_std_reference
    LABELS reference
    TIMEOUT 30
)
