cmake_minimum_required(VERSION 3.28)

project(C1_odr_review_symbol_missing LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/StudySetup.cmake")

add_library(C1_odr_observation_objects OBJECT
    observer_a.cpp
    observer_b.cpp
)
target_include_directories(C1_odr_observation_objects PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
engineering_configure_target(C1_odr_observation_objects)

get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
find_program(C1_ODR_DUMPBIN dumpbin HINTS "${_compiler_dir}")
find_program(C1_ODR_NM nm)

engineering_add_test(C1_odr_observation_symbols
    COMMAND
        ${CMAKE_COMMAND}
        -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DDUMPBIN=${C1_ODR_DUMPBIN}
        -DNM=${C1_ODR_NM}
        -P ${CMAKE_CURRENT_LIST_DIR}/../../cmake/observe_symbols.cmake
    LABELS review-negative
    TIMEOUT 30
)

