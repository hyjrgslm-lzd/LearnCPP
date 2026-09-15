cmake_minimum_required(VERSION 3.28)
project(concurrency_runtime_tests LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
if(BUILD_TESTING)
    if(CONCURRENCY_STUDY_TEST_FAST_MATH)
        if(NOT MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
            message(FATAL_ERROR "The isolated fast-math experiment needs verified compiler-specific flags")
        endif()
        set(_cs_simd_sources "${CONCURRENCY_STUDY_EXERCISES_DIR}/../topics/simd")
        add_library(numeric_fast_kernel OBJECT "${_cs_simd_sources}/fast_math_kernel.cpp")
        cs_configure_target(numeric_fast_kernel)
        cs_add_visible_files(numeric_fast_kernel "${_cs_simd_sources}/fast_math_api.hpp")
        add_executable(numeric_fast_check "${_cs_simd_sources}/fast_math_check.cpp")
        cs_configure_target(numeric_fast_check)
        cs_add_visible_files(numeric_fast_check "${_cs_simd_sources}/fast_math_api.hpp"
            "${_cs_simd_sources}/fast-math-build.md")
        target_link_libraries(numeric_fast_check PRIVATE numeric_fast_kernel)
        set_target_properties(numeric_fast_kernel numeric_fast_check PROPERTIES INTERPROCEDURAL_OPTIMIZATION FALSE)
        foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES CMAKE_BUILD_TYPE)
            string(TOUPPER "${config}" upper_config)
            set_target_properties(numeric_fast_kernel numeric_fast_check
                PROPERTIES INTERPROCEDURAL_OPTIMIZATION_${upper_config} FALSE)
        endforeach()
        if(MSVC)
            target_compile_options(numeric_fast_kernel PRIVATE /fp:fast /GL-)
            target_compile_options(numeric_fast_check PRIVATE /fp:strict /GL-)
            target_link_options(numeric_fast_check PRIVATE /LTCG:OFF /OPT:NOICF)
        else()
            target_compile_options(numeric_fast_kernel PRIVATE -ffast-math -fno-lto)
            target_compile_options(numeric_fast_check PRIVATE -fno-fast-math -ffp-contract=off -fno-lto)
            target_link_options(numeric_fast_check PRIVATE -fno-fast-math -fno-lto)
        endif()
        add_test(NAME numeric_fast_check COMMAND numeric_fast_check)
        set_tests_properties(numeric_fast_check PROPERTIES TIMEOUT ${CS_TEST_TIMEOUT} LABELS "runtime;fast-math")
    endif()
    file(GLOB test_sources CONFIGURE_DEPENDS "*_test.cpp")
    foreach(source IN LISTS test_sources)
        get_filename_component(stem "${source}" NAME_WE)
        add_executable(runtime_${stem} "${source}")
        cs_configure_target(runtime_${stem})
        set(_cs_runtime_headers)
        if(stem STREQUAL "numeric_test")
            list(APPEND _cs_runtime_headers
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/J1_false_sharing/reference.hpp")
        elseif(stem STREQUAL "reclamation_test")
            list(APPEND _cs_runtime_headers
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/../topics/reclamation/experiment_support.hpp"
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/I2_hazard_pointer/reference.hpp"
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/I3_rcu/reference.hpp"
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/R1_epoch_reclamation/reference.hpp"
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/R2_qsbr/reference.hpp")
        elseif(stem STREQUAL "thread_pool_test")
            list(APPEND _cs_runtime_headers
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/Capstone1_thread_pool/checks.hpp"
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/tools/test_tools.py"
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/tools/verify_materials.py"
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/tools/verify_students.py"
                "${CONCURRENCY_STUDY_EXERCISES_DIR}/validation/README.md")
        endif()
        cs_add_visible_files(runtime_${stem} ${_cs_runtime_headers})
        add_test(NAME runtime_${stem} COMMAND runtime_${stem})
        set_tests_properties(runtime_${stem} PROPERTIES TIMEOUT ${CS_TEST_TIMEOUT} LABELS "runtime" SKIP_RETURN_CODE 77)
    endforeach()
    if(EXISTS "${CONCURRENCY_STUDY_EXERCISES_DIR}/../topics/reclamation/lifetime_reference.cpp")
        add_executable(runtime_lifetime_reference "${CONCURRENCY_STUDY_EXERCISES_DIR}/../topics/reclamation/lifetime_reference.cpp")
        cs_configure_target(runtime_lifetime_reference)
        cs_add_visible_files(runtime_lifetime_reference
            "${CONCURRENCY_STUDY_EXERCISES_DIR}/../topics/reclamation/experiment_support.hpp")
        add_test(NAME runtime_lifetime_reference COMMAND runtime_lifetime_reference)
        set_tests_properties(runtime_lifetime_reference PROPERTIES TIMEOUT ${CS_TEST_TIMEOUT} LABELS "runtime")
    endif()
    # Missing/malformed interpreter paths must not silently omit tool checks.
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    add_test(NAME runtime_benchmark_tools COMMAND ${Python3_EXECUTABLE}
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/tools/test_tools.py")
    set_tests_properties(runtime_benchmark_tools PROPERTIES TIMEOUT 30 LABELS "tools")
    add_test(NAME runtime_materials COMMAND ${Python3_EXECUTABLE}
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/tools/verify_materials.py")
    set_tests_properties(runtime_materials PROPERTIES TIMEOUT 30 LABELS "tools")
endif()
if(COMMAND learncpp_setup_ide)
    learncpp_setup_ide("")
endif()
