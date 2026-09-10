cmake_minimum_required(VERSION 3.28)
project(P1_file_pipeline LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

if(NOT EXISTS "${C07_EXERCISES}/include/c07/file_pipeline_io.hpp")
    message(FATAL_ERROR "P1_file_pipeline requires include/c07/file_pipeline_io.hpp")
endif()

set(c07_p1_libs)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND C07_STUDY_ENABLE_IO_URING AND TARGET c07_liburing)
    list(APPEND c07_p1_libs c07_liburing)
endif()

c07_add_exercise(NAME P1_file_pipeline HEADER pipeline.hpp CHECK checks/pipeline_checks.cpp
    BAD_DIAGNOSTIC "out-of-order chunks assemble by offset"
    LIBRARIES ${c07_p1_libs})

if(C07_STUDY_BUILD_REFERENCE)
    add_executable(P1_file_pipeline_demo checks/pipeline_demo.cpp)
    c07_configure_target(P1_file_pipeline_demo)
    target_include_directories(P1_file_pipeline_demo PRIVATE src/reference checks)
    if(c07_p1_libs)
        target_link_libraries(P1_file_pipeline_demo PRIVATE ${c07_p1_libs})
    endif()
    c07_add_test(P1_file_pipeline_demo_self_test COMMAND "$<TARGET_FILE:P1_file_pipeline_demo>" --self-test
        LABELS reference)
endif()

if(C07_STUDY_BUILD_REFERENCE)
    foreach(backend buffered mapped)
        c07_add_test(P1_file_pipeline_reference_${backend}
            COMMAND "$<TARGET_FILE:P1_file_pipeline_reference>" --backend ${backend}
            LABELS reference platform)
        c07_add_test(P1_file_pipeline_validation_good_${backend}
            COMMAND "$<TARGET_FILE:P1_file_pipeline_validation_good>" --backend ${backend}
            LABELS validation_good platform)
    endforeach()

    set(c07_p1_completion_enabled OFF)
    if(WIN32)
        set(c07_p1_completion_enabled ON)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND C07_STUDY_ENABLE_IO_URING AND TARGET c07_liburing)
        set(c07_p1_completion_enabled ON)
    endif()
    if(c07_p1_completion_enabled)
        add_executable(P1_native_failure checks/native_failure_checks.cpp)
        c07_configure_target(P1_native_failure)
        target_compile_definitions(P1_native_failure PRIVATE C07_PIPELINE_FAILURE_TEST=1)
        if(c07_p1_libs)
            target_link_libraries(P1_native_failure PRIVATE ${c07_p1_libs})
        endif()
        c07_add_test(P1_native_failure LABELS validation platform ALLOW_SKIP)
        c07_add_test(P1_file_pipeline_reference_completion
            COMMAND "$<TARGET_FILE:P1_file_pipeline_reference>" --backend completion
            LABELS reference platform ALLOW_SKIP)
        c07_add_test(P1_file_pipeline_validation_good_completion
            COMMAND "$<TARGET_FILE:P1_file_pipeline_validation_good>" --backend completion
            LABELS validation_good platform ALLOW_SKIP)
    else()
        message(STATUS "P1_file_pipeline completion backend tests NOT_ENABLED")
    endif()
endif()
