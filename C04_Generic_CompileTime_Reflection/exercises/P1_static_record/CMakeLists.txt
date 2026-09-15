cmake_minimum_required(VERSION 3.28)
project(P1_static_record LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
c04_add_exercise(NAME P1_static_record HEADER record_ops.hpp CHECK checks/record_checks.cpp
    BAD_DIAGNOSTIC "encoded fields cover the complete schema in declaration order")
if(TARGET P1_static_record_reference)
    target_sources(P1_static_record_reference PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/reference/codec.hpp")
endif()
if(TARGET P1_static_record_validation_good)
    target_sources(P1_static_record_validation_good PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/validation/good/codec_good.hpp")
endif()
if(TARGET P1_static_record_validation_bad)
    target_sources(P1_static_record_validation_bad PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad/codec_bad.hpp")
endif()
c04_add_observation(P1_static_record_known_baseline observations/known_record.cpp)
target_include_directories(P1_static_record_known_baseline PRIVATE checks)
target_sources(P1_static_record_known_baseline PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/checks/record_schema.hpp")
if(GENERIC_STUDY_BUILD_REFERENCE)
    c04_add_observation(P1_static_record_schema_gap observations/manual_schema_gap.cpp)
    target_include_directories(P1_static_record_schema_gap PRIVATE src/reference checks)
    target_sources(P1_static_record_schema_gap PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/checks/record_schema.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/reference/codec.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/reference/record_ops.hpp")
endif()
if(GENERIC_STUDY_ENABLE_FRONTIER AND GENERIC_STUDY_BUILD_REFERENCE)
    add_executable(P1_static_record_reflection checks/reflection_driver.cpp)
    c04_configure_target(P1_static_record_reflection)
    target_include_directories(P1_static_record_reflection PRIVATE src/reflection checks)
    if(MSVC)
        target_compile_options(P1_static_record_reflection PRIVATE /std:c++latest)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(P1_static_record_reflection PRIVATE -std=c++26 -freflection)
    else()
        target_compile_options(P1_static_record_reflection PRIVATE -std=c++2c)
    endif()
    c04_add_test(P1_static_record_reflection LABELS capability)
    if(BUILD_TESTING)
        set_tests_properties(P1_static_record_reflection PROPERTIES SKIP_RETURN_CODE 77)
    endif()
endif()

learncpp_setup_ide(P1_static_record_student)
