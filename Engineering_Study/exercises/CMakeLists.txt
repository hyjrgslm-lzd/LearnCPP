cmake_minimum_required(VERSION 3.28)

option(ENGINEERING_STUDY_ENABLE_IMPORT_STD "Build the separately gated import std lesson" OFF)
if(ENGINEERING_STUDY_ENABLE_IMPORT_STD)
    if(NOT CMAKE_VERSION VERSION_EQUAL "4.2.3")
        message(FATAL_ERROR "This import std experiment is pinned to CMake 4.2.3. Check the exact installed version's Help/dev/experimental.rst before porting the gate.")
    endif()
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
endif()

project(engineering_study LANGUAGES C CXX)
include(cmake/StudySetup.cmake)

add_executable(engineering_check_selftest cmake/check_selftest.cpp)
engineering_configure_target(engineering_check_selftest)
engineering_add_test(engineering_check_success COMMAND engineering_check_selftest LABELS tooling)
engineering_add_test(engineering_check_failure
    COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:engineering_check_selftest>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/verify_check_failure.cmake
    LABELS tooling)

foreach(unit IN ITEMS A1_build_debug B1_preprocessor C1_odr C2_archive
                      D1_shared_library E1_abi F1_cmake_targets F2_dependencies
                      G1_diagnostics G2_build_cost J1_package)
    add_subdirectory(${unit})
endforeach()
if(ENGINEERING_STUDY_ENABLE_MODULES)
    add_subdirectory(H1_modules)
endif()
if(ENGINEERING_STUDY_ENABLE_IMPORT_STD)
    add_subdirectory(I1_import_std)
endif()

message(STATUS "C01 named modules requested: ${ENGINEERING_STUDY_ENABLE_MODULES}")
message(STATUS "C01 import std requested: ${ENGINEERING_STUDY_ENABLE_IMPORT_STD}")
message(STATUS "Options OFF mean not requested, not a successful capability probe.")
