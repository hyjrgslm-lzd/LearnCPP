cmake_minimum_required(VERSION 3.28)

project(C1_odr_negative_duplicate_header_definition LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(C1_odr_negative_duplicate_header_definition_objects OBJECT
    caller_a.cpp
    caller_b.cpp
)

add_executable(C1_odr_negative_duplicate_header_definition_link
    $<TARGET_OBJECTS:C1_odr_negative_duplicate_header_definition_objects>
    main.cpp
)
