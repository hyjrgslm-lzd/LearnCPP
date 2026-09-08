cmake_minimum_required(VERSION 3.28)
project(G2_build_cost LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

set(G2_SOURCES
    reference/alpha.cpp
    reference/beta.cpp
    reference/gamma.cpp
    reference/common.cpp
    reference/main.cpp
)

function(g2_add_variant name)
    add_executable(${name} ${G2_SOURCES})
    target_include_directories(${name} PRIVATE reference/include)
    engineering_configure_target(${name})
endfunction()

g2_add_variant(G2_build_cost_baseline)
g2_add_variant(G2_build_cost_pch)
target_precompile_headers(G2_build_cost_pch PRIVATE reference/include/g2_common/heavy.hpp)

g2_add_variant(G2_build_cost_lto)
set_property(TARGET G2_build_cost_lto PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

foreach(target IN ITEMS G2_build_cost_baseline G2_build_cost_pch G2_build_cost_lto)
    engineering_add_test(${target}
        COMMAND ${target}
        LABELS reference
        TIMEOUT 30
    )
endforeach()
