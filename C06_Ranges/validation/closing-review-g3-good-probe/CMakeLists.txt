cmake_minimum_required(VERSION 3.28)
project(closing_review_g3_good_probe LANGUAGES CXX)

if(MSVC)
    set(CMAKE_CXX26_STANDARD_COMPILE_OPTION "/std:c++latest")
    set(CMAKE_CXX26_EXTENSION_COMPILE_OPTION "/std:c++latest")
endif()

add_executable(closing_review_g3_good_probe main.cpp)
set_target_properties(closing_review_g3_good_probe PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_SCAN_FOR_MODULES OFF)
target_include_directories(closing_review_g3_good_probe PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../exercises/G3_my_enumerate_borrowed/validation/good")
if(MSVC)
    target_compile_options(closing_review_g3_good_probe PRIVATE /utf-8 /EHsc /permissive- /Zc:__cplusplus)
endif()
