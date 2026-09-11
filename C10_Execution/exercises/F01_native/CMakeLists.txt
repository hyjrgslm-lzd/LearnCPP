cmake_minimum_required(VERSION 3.28)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(C10_F01_native LANGUAGES CXX)
    include("../cmake/StudySetup.cmake")
endif()

function(c10_f01_mode_flags out_var)
    set(mode_option "CMAKE_CXX${C10_CXX_STANDARD}_STANDARD_COMPILE_OPTION")
    if(NOT DEFINED ${mode_option})
        message(FATAL_ERROR "No compiler mode mapping for requested C++${C10_CXX_STANDARD}")
    endif()
    set(flags "${${mode_option}}")
    if(MSVC)
        string(APPEND flags " /Zc:__cplusplus")
    endif()
    set(${out_var} "${flags}" PARENT_SCOPE)
endfunction()

function(c10_f01_probe name source)
    c10_f01_mode_flags(C10_F01_MODE_FLAGS)
    if(WIN32)
        separate_arguments(C10_F01_MODE_FLAG_LIST WINDOWS_COMMAND "${C10_F01_MODE_FLAGS}")
    else()
        separate_arguments(C10_F01_MODE_FLAG_LIST UNIX_COMMAND "${C10_F01_MODE_FLAGS}")
    endif()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/probe-output")
    try_compile(C10_F01_${name}_OK
        SOURCE_FROM_FILE "${name}_probe.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/probes/${source}"
        COMPILE_DEFINITIONS ${C10_F01_MODE_FLAG_LIST}
        OUTPUT_VARIABLE C10_F01_${name}_OUTPUT)
    if(C10_F01_${name}_OK)
        set(C10_F01_${name}_VALUE 1 PARENT_SCOPE)
    else()
        set(C10_F01_${name}_VALUE 0 PARENT_SCOPE)
    endif()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/probe-output/${name}.txt"
        "${C10_F01_${name}_OUTPUT}")
endfunction()

c10_f01_mode_flags(C10_F01_MODE_FLAGS)
c10_f01_probe(BASELINE baseline.cpp)
if(NOT C10_F01_BASELINE_VALUE)
    message(FATAL_ERROR "F01 compiler baseline failed; see ${CMAKE_CURRENT_BINARY_DIR}/probe-output/BASELINE.txt")
endif()
c10_f01_probe(CORE core.cpp)
c10_f01_probe(SCHEDULER scheduler.cpp)
c10_f01_probe(TASK task.cpp)
c10_f01_probe(SCOPE scope.cpp)
c10_f01_probe(BULK bulk.cpp)

configure_file(probe_config.hpp.in probe_config.hpp @ONLY)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/probe-summary.txt"
    "requested_standard=${C10_CXX_STANDARD}\n"
    "required_flags=${C10_F01_MODE_FLAGS}\n"
    "baseline=${C10_F01_BASELINE_VALUE}\n"
    "core=${C10_F01_CORE_VALUE}\n"
    "scheduler=${C10_F01_SCHEDULER_VALUE}\n"
    "task=${C10_F01_TASK_VALUE}\n"
    "scope=${C10_F01_SCOPE_VALUE}\n"
    "bulk=${C10_F01_BULK_VALUE}\n")

foreach(unit IN ITEMS core scheduler task scope bulk)
    c10_add_observation(F01_native_${unit} "body/${unit}.cpp" ALLOW_SKIP LABELS native standard)
    target_include_directories(F01_native_${unit} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
endforeach()
