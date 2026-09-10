cmake_minimum_required(VERSION 3.28)
project(L08_avl_tree LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME L08_avl_tree
    HEADER avl_set.hpp
    CHECK checks/avl_checks.cpp
    BAD_DIAGNOSTIC "lr rotation keeps sorted balanced tree"
)

if(RANGES_BUILD_REFERENCE)
    foreach(extra_bad IN ITEMS bad_vector bad_stale_height)
        set(target "L08_avl_tree_validation_${extra_bad}")
        add_executable(${target} checks/avl_checks.cpp)
        ranges_configure_target(${target})
        target_include_directories(${target} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/validation/${extra_bad}"
            "${CMAKE_CURRENT_SOURCE_DIR}/checks")
        if(extra_bad STREQUAL "bad_vector")
            set(expected_text "structural root is present")
        else()
            set(expected_text "stored height matches actual height")
        endif()
        ranges_add_test(${target}_rejected COMMAND ${CMAKE_COMMAND}
            "-DEXECUTABLE=$<TARGET_FILE:${target}>"
            "-DEXPECTED_TEXT=${expected_text}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../../../C05_Data_Representation_Standard_Facilities/exercises/cmake/expect_failure.cmake"
            LABELS negative)
    endforeach()
endif()
