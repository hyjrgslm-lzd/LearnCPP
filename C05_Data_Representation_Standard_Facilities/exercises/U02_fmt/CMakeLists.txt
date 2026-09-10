cmake_minimum_required(VERSION 3.28)
project(U02_fmt LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
if(NOT DATA_STUDY_ENABLE_FORMAT_LIBS)
    message(STATUS "U02_fmt disabled; DATA_STUDY_ENABLE_FORMAT_LIBS=OFF")
    return()
endif()
include(../cmake/FormatLibraries.cmake)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

add_executable(U02_fmt_observation observation.cpp)
c05_configure_target(U02_fmt_observation)
c05_link_fmt(U02_fmt_observation)
c05_add_test(U02_fmt_observation LABELS observation)

set(_diag_setup "${CMAKE_CURRENT_BINARY_DIR}/diagnostics/fmt_diagnostic_setup.cmake")
file(WRITE "${_diag_setup}"
    "set(DATA_STUDY_ENABLE_FORMAT_LIBS ON CACHE BOOL \"\" FORCE)\n"
    "include(\"${CMAKE_CURRENT_LIST_DIR}/../cmake/FormatLibraries.cmake\")\n"
    "c05_link_fmt(control)\n"
    "c05_link_fmt(subject)\n")
string(SHA256 _diag_key "${CMAKE_BINARY_DIR}|U02_fmt_bad_compile_format_rejected")
string(SUBSTRING "${_diag_key}" 0 12 _diag_key)
set(_diag_build "${CMAKE_CURRENT_LIST_DIR}/../build/_diag/${_diag_key}/$<CONFIG>")
c05_add_test(U02_fmt_bad_compile_format_rejected COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_LIST_DIR}/../../../C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}/diagnostics/bad_compile_format.cpp"
    --control "${CMAKE_CURRENT_SOURCE_DIR}/diagnostics/good_compile_format.cpp"
    --pattern "C7595|即时函数"
    --build "${_diag_build}"
    --generator "${CMAKE_GENERATOR}" "--platform=${CMAKE_GENERATOR_PLATFORM}"
    --config "$<CONFIG>" --compiler "${CMAKE_CXX_COMPILER}" --setup "${_diag_setup}"
    LABELS diagnostic TIMEOUT 420)
set_tests_properties(U02_fmt_bad_compile_format_rejected PROPERTIES RUN_SERIAL TRUE)
