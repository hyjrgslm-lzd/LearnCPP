cmake_minimum_required(VERSION 3.28)
project(CAPSTONE3_impl_source_reading LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_observation(CAPSTONE3_impl_source_reading main.cpp)


learncpp_setup_ide(CAPSTONE3_impl_source_reading
    FILES
        notes/01_object_diagram.md
        notes/02_pattern_table.md
        notes/03_cpo_checklist.md
        notes/04_reading_note.md)
