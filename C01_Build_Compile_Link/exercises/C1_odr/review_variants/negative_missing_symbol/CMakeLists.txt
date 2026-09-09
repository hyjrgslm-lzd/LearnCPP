cmake_minimum_required(VERSION 3.28)

project(C1_odr_review_negative_missing_symbol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(C1_odr_review_negative_missing_symbol_objects OBJECT
    caller_a.cpp
    caller_b.cpp
)

add_executable(C1_odr_review_negative_missing_symbol_link
    $<TARGET_OBJECTS:C1_odr_review_negative_missing_symbol_objects>
    main.cpp
)
