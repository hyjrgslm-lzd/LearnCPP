cmake_minimum_required(VERSION 3.28)
project(CAPSTONE4_mini_ranges LANGUAGES CXX)
include(../cmake/RangesSetup.cmake)

ranges_add_exercise(
    NAME CAPSTONE4_mini_ranges
    HEADER my_ranges/06_consumers.hpp
    CHECK main.cpp
    BAD_DIAGNOSTIC "take_view stops after requested count"
)

set(capstone4_headers
    my_ranges/01_cpo.hpp
    my_ranges/02_concepts.hpp
    my_ranges/03_interface.hpp
    my_ranges/04_factories.hpp
    my_ranges/05_adaptors.hpp
    my_ranges/06_consumers.hpp)
foreach(variant IN ITEMS student reference validation_good validation_bad)
    if(TARGET CAPSTONE4_mini_ranges_${variant})
        if(variant MATCHES "^validation_(.*)")
            set(root "validation/${CMAKE_MATCH_1}")
        else()
            set(root "src/${variant}")
        endif()
        foreach(header IN LISTS capstone4_headers)
            target_sources(CAPSTONE4_mini_ranges_${variant} PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/${root}/${header}")
        endforeach()
    endif()
endforeach()

learncpp_setup_ide(CAPSTONE4_mini_ranges_student
    FILES notes/architecture_diagram.md notes/design_doc.md)
