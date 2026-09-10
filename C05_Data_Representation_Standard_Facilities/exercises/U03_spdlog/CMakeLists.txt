cmake_minimum_required(VERSION 3.28)
project(U03_spdlog LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
if(NOT DATA_STUDY_ENABLE_FORMAT_LIBS)
    message(STATUS "U03_spdlog disabled; DATA_STUDY_ENABLE_FORMAT_LIBS=OFF")
    return()
endif()
include(../cmake/FormatLibraries.cmake)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

foreach(active_level DEBUG INFO)
    add_executable(U03_spdlog_${active_level} observation.cpp)
    c05_configure_target(U03_spdlog_${active_level})
    target_compile_definitions(U03_spdlog_${active_level} PRIVATE
        "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${active_level}")
    c05_link_spdlog(U03_spdlog_${active_level})
    c05_add_test(U03_spdlog_${active_level} LABELS observation)
endforeach()

set(_diag_setup "${CMAKE_CURRENT_BINARY_DIR}/diagnostics/spdlog_diagnostic_setup.cmake")
file(WRITE "${_diag_setup}"
    "set(DATA_STUDY_ENABLE_FORMAT_LIBS ON CACHE BOOL \"\" FORCE)\n"
    "set(DATA_STUDY_SPDLOG_BACKEND \"${DATA_STUDY_SPDLOG_BACKEND}\" CACHE STRING \"\" FORCE)\n"
    "include(\"${CMAKE_CURRENT_LIST_DIR}/../cmake/FormatLibraries.cmake\")\n"
    "c05_link_spdlog(control)\n"
    "c05_link_spdlog(subject)\n"
    "target_compile_definitions(control PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)\n"
    "target_compile_definitions(subject PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)\n")
string(SHA256 _diag_key "${CMAKE_BINARY_DIR}|U03_spdlog_bad_compile_format_rejected")
string(SUBSTRING "${_diag_key}" 0 12 _diag_key)
set(_diag_build "${CMAKE_CURRENT_LIST_DIR}/../build/_diag/${_diag_key}/$<CONFIG>")
c05_add_test(U03_spdlog_bad_compile_format_rejected COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_LIST_DIR}/../../../C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}/diagnostics/bad_compile_format.cpp"
    --control "${CMAKE_CURRENT_SOURCE_DIR}/diagnostics/good_compile_format.cpp"
    --pattern "C7595|即时函数"
    --build "${_diag_build}"
    --generator "${CMAKE_GENERATOR}" "--platform=${CMAKE_GENERATOR_PLATFORM}"
    --config "$<CONFIG>" --compiler "${CMAKE_CXX_COMPILER}" --setup "${_diag_setup}"
    LABELS diagnostic TIMEOUT 420)
set_tests_properties(U03_spdlog_bad_compile_format_rejected PROPERTIES RUN_SERIAL TRUE)
