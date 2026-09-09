cmake_minimum_required(VERSION 3.28)
project(P1_document LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c03_add_exercise(NAME P1_document HEADER document.hpp CHECK checks/document_checks.cpp
    BAD_DIAGNOSTIC "failed batch preserves the entire original document")

if(TYPE_STUDY_BUILD_REFERENCE)
    foreach(variant reference good)
        set(target P1_document_allocation_${variant})
        add_executable(${target} checks/document_checks.cpp)
        c03_configure_target(${target})
        if(variant STREQUAL "reference")
            set(implementation src/reference)
        else()
            set(implementation validation/good)
        endif()
        target_include_directories(${target} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/${implementation}" "${CMAKE_CURRENT_SOURCE_DIR}/checks")
        target_compile_definitions(${target} PRIVATE C03_ALLOCATION_INJECTION=1)
        if(MSVC)
            # Global allocation failure must not hit noexcept Debug STL proxy bookkeeping.
            # This changes only these single-TU experiments, not the normal Debug tests.
            target_compile_definitions(${target} PRIVATE _ITERATOR_DEBUG_LEVEL=0)
        endif()
        c03_add_test(${target} LABELS allocation)
    endforeach()
endif()
