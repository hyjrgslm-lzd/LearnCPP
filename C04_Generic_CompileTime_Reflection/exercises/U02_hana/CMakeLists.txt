cmake_minimum_required(VERSION 3.28)
project(U02_hana LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

if(NOT GENERIC_STUDY_ENABLE_META_LIBS)
    message(STATUS "U02_hana disabled: set GENERIC_STUDY_ENABLE_META_LIBS=ON after running tools/prepare_meta_libraries.ps1")
    return()
endif()

include(../cmake/MetaLibraries.cmake)

c04_add_observation(U02_hana_record_observation observations/hana_record_observation.cpp)
c04_add_observation(U02_hana_migration_solution observations/hana_migration_solution.cpp)

foreach(target U02_hana_record_observation U02_hana_migration_solution)
    c04_link_hana(${target})
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../A02_field_projection/checks")
endforeach()
