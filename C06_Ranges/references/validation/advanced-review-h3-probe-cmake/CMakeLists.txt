cmake_minimum_required(VERSION 3.28)
project(advanced_review_h3_probe LANGUAGES CXX)

if(MSVC)
    set(CMAKE_CXX26_STANDARD_COMPILE_OPTION "/std:c++latest")
    set(CMAKE_CXX26_EXTENSION_COMPILE_OPTION "/std:c++latest")
endif()

add_executable(h3_hostile_probe ../advanced-review-h3-hostile-probe.cpp)
set_target_properties(h3_hostile_probe PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(h3_hostile_probe PRIVATE /utf-8 /EHsc /permissive- /Zc:__cplusplus)
endif()
target_include_directories(h3_hostile_probe PRIVATE
    ../../../exercises/H3_generator_const_iter/src/reference
    ../../../exercises/H3_generator_const_iter/checks
    ../../../exercises/include
    ../../../../C01_Build_Compile_Link/exercises/include)
