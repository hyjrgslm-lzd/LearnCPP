include_guard(GLOBAL)

include(CheckCXXCompilerFlag)

function(coroutine_study_enable_cxx26_preview target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /std:c++latest)
        return()
    endif()

    check_cxx_compiler_flag("-std=c++2c" COROUTINE_STUDY_HAS_CXX2C_FLAG)
    if(COROUTINE_STUDY_HAS_CXX2C_FLAG)
        target_compile_options(${target} PRIVATE -std=c++2c)
    else()
        message(FATAL_ERROR "${target} requires a compiler with a C++26 preview flag.")
    endif()
endfunction()

# H exercises share an immutable checker and substitute only the candidate header.
function(coroutine_study_add_checker_variants name)
    if(NOT COROUTINE_STUDY_BUILD_REFERENCE)
        return()
    endif()
    foreach(variant good bad_constant)
        add_executable(${name}_${variant} checks/main.cpp)
        target_compile_features(${name}_${variant} PRIVATE cxx_std_23)
        target_include_directories(${name}_${variant} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/validation/${variant}"
            "${COROUTINE_STUDY_EXERCISES_DIR}/include")
        target_link_libraries(${name}_${variant} PRIVATE stdexec::stdexec)
    endforeach()
    if(BUILD_TESTING)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        add_test(NAME ${name}_good COMMAND ${name}_good)
        coroutine_study_set_test_defaults(${name}_good "good;checker;stdexec" 30)
        add_test(NAME ${name}_bad_constant_rejected COMMAND ${Python3_EXECUTABLE}
            "${COROUTINE_STUDY_EXERCISES_DIR}/../../C07_OS_Memory_System_IO/exercises/tools/run_test.py"
            --name ${name}_bad_constant --records "${CMAKE_CURRENT_BINARY_DIR}/validation"
            --timeout 25 --expect-exit 1 --contains "student check failed"
            -- $<TARGET_FILE:${name}_bad_constant>)
        coroutine_study_set_test_defaults(${name}_bad_constant_rejected "bad;checker;stdexec" 30)
    endif()
endfunction()

function(coroutine_study_set_test_defaults name label timeout)
    if(TEST ${name})
        set_tests_properties(${name} PROPERTIES
            LABELS "${label}"
            TIMEOUT ${timeout}
        )
    endif()
endfunction()

function(coroutine_study_add_exercise name)
    cmake_parse_arguments(ARG "STUDENT_TEST" "STANDARD" "SOURCES;REFERENCE_SOURCES;LIBRARIES" ${ARGN})
    if(NOT ARG_SOURCES)
        set(ARG_SOURCES main.cpp)
    endif()
    if(NOT ARG_STANDARD)
        set(ARG_STANDARD 23)
    endif()

    add_executable(${name} ${ARG_SOURCES})
    target_compile_features(${name} PRIVATE cxx_std_${ARG_STANDARD})
    if(NOT COROUTINE_STUDY_EXERCISES_DIR)
        message(FATAL_ERROR "COROUTINE_STUDY_EXERCISES_DIR must point to C09_Coroutines/exercises")
    endif()
    target_include_directories(${name} PRIVATE "${COROUTINE_STUDY_EXERCISES_DIR}/include")
    if(ARG_LIBRARIES)
        target_link_libraries(${name} PRIVATE ${ARG_LIBRARIES})
    endif()
    if(BUILD_TESTING AND COROUTINE_STUDY_TEST_STARTERS AND ARG_STUDENT_TEST)
        add_test(NAME ${name} COMMAND ${name})
        coroutine_study_set_test_defaults(${name} starter 30)
    endif()
    if(COROUTINE_STUDY_BUILD_REFERENCE AND ARG_REFERENCE_SOURCES)
        add_executable(${name}_reference ${ARG_REFERENCE_SOURCES})
        target_compile_features(${name}_reference PRIVATE cxx_std_${ARG_STANDARD})
        target_include_directories(${name}_reference PRIVATE "${COROUTINE_STUDY_EXERCISES_DIR}/include")
        if(ARG_LIBRARIES)
            target_link_libraries(${name}_reference PRIVATE ${ARG_LIBRARIES})
        endif()
        if(BUILD_TESTING)
            add_test(NAME ${name}_reference COMMAND ${name}_reference)
            coroutine_study_set_test_defaults(${name}_reference reference 30)
        endif()
    endif()
endfunction()
