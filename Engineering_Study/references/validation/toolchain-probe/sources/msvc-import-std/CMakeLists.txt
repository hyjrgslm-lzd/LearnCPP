cmake_minimum_required(VERSION 4.2)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
project(msvc_import_std_probe LANGUAGES CXX)

add_executable(import_std_probe src/main.cpp)
target_compile_features(import_std_probe PRIVATE cxx_std_23)
set_property(TARGET import_std_probe PROPERTY CXX_MODULE_STD ON)

if(NOT CMAKE_CXX_COMPILER_IMPORT_STD)
    message(FATAL_ERROR "CMAKE_CXX_COMPILER_IMPORT_STD is empty")
endif()
if(NOT 23 IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
    message(FATAL_ERROR "CMAKE_CXX_COMPILER_IMPORT_STD does not include 23")
endif()
