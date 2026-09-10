cmake_minimum_required(VERSION 3.28)
project(L08_completion LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
set(c07_l08_libs)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND C07_STUDY_ENABLE_IO_URING AND TARGET c07_liburing)
    list(APPEND c07_l08_libs c07_liburing)
endif()
c07_add_exercise(NAME L08_completion HEADER completion.hpp CHECK checks/completion_checks.cpp
    BAD_DIAGNOSTIC "missing target completion is rejected"
    LIBRARIES ${c07_l08_libs})
if(TARGET c07_liburing)
    foreach(target L08_completion_student L08_completion_reference L08_completion_validation_good L08_completion_validation_bad)
        if(TARGET ${target})
            target_compile_definitions(${target} PRIVATE C07_HAS_LIBURING=1)
        endif()
    endforeach()
endif()

set(c07_l08_platform_enabled OFF)
if(WIN32)
    set(c07_l08_platform_enabled ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND C07_STUDY_ENABLE_IO_URING AND TARGET c07_liburing)
    set(c07_l08_platform_enabled ON)
endif()

if(c07_l08_platform_enabled AND C07_STUDY_BUILD_REFERENCE)
    c07_add_test(L08_completion_reference_platform
        COMMAND "$<TARGET_FILE:L08_completion_reference>" --platform
        LABELS reference platform ALLOW_SKIP)
    c07_add_test(L08_completion_validation_good_platform
        COMMAND "$<TARGET_FILE:L08_completion_validation_good>" --platform
        LABELS validation_good platform ALLOW_SKIP)
else()
    message(STATUS "L08_completion platform tests NOT_ENABLED")
endif()
