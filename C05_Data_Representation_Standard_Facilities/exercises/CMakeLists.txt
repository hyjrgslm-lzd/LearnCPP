cmake_minimum_required(VERSION 3.28)
project(C05_Data_Representation_Standard_Facilities LANGUAGES CXX)
include(cmake/StudySetup.cmake)
foreach(unit
    L01_bytes
    L02_bits_endian
    L03_integer_boundaries
    L04_strings_views
    L05_unicode
    L06_transcoding
    L07_text_semantics
    L08_parsing
    L09_formatting
    L10_clocks_durations
    L11_calendar_zones
    L12_paths
    L13_configuration
    L14_binary_fields
    L15_schema_evolution
    P1_resource_manifest
    U01_icu_unicode
    U02_fmt
    U03_spdlog
    F01_frontier
)
    add_subdirectory(${unit})
endforeach()
