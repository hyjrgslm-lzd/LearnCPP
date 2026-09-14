cmake_minimum_required(VERSION 3.28)

project(C1_odr LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

add_executable(C1_odr_student
    checks/student_check.cpp
    support/student_callers/caller_a.cpp
    support/student_callers/caller_b.cpp
    src/student/lesson.cpp
)
target_include_directories(C1_odr_student PRIVATE src/student)
engineering_configure_target(C1_odr_student)
target_sources(C1_odr_student PRIVATE src/student/lesson.hpp)

if(ENGINEERING_STUDY_BUILD_REFERENCE)
    add_executable(C1_odr_reference
        checks/reference_check.cpp
        reference/split_definition/caller_a.cpp
        reference/split_definition/caller_b.cpp
        reference/split_definition/lesson.cpp
    )
    target_include_directories(C1_odr_reference PRIVATE reference/split_definition)
    engineering_configure_target(C1_odr_reference)
    target_sources(C1_odr_reference PRIVATE reference/split_definition/lesson.hpp)

    add_executable(C1_odr_reference_inline
        reference/inline_definition/main.cpp
        reference/inline_definition/caller_a.cpp
        reference/inline_definition/caller_b.cpp
    )
    target_include_directories(C1_odr_reference_inline PRIVATE reference/inline_definition)
    engineering_configure_target(C1_odr_reference_inline)
    target_sources(C1_odr_reference_inline PRIVATE reference/inline_definition/lesson.hpp)
endif()

add_executable(C1_odr_observation_single
    observation/single_consumer/main.cpp
)
target_include_directories(C1_odr_observation_single PRIVATE observation/single_consumer)
engineering_configure_target(C1_odr_observation_single)
target_sources(C1_odr_observation_single PRIVATE observation/single_consumer/lesson.hpp)

add_library(C1_odr_observation_objects OBJECT
    observation/two_tu_symbols/observer_a.cpp
    observation/two_tu_symbols/observer_b.cpp
)
target_include_directories(C1_odr_observation_objects PRIVATE observation/two_tu_symbols)
engineering_configure_target(C1_odr_observation_objects)
target_sources(C1_odr_observation_objects PRIVATE observation/two_tu_symbols/lesson.hpp)

if(ENGINEERING_STUDY_BUILD_REFERENCE)
    engineering_add_test(C1_odr_reference
        COMMAND C1_odr_reference
        LABELS reference
        TIMEOUT 30
    )
    engineering_add_test(C1_odr_reference_inline
        COMMAND C1_odr_reference_inline
        LABELS reference
        TIMEOUT 30
    )
endif()

if(ENGINEERING_STUDY_TEST_STUDENTS)
    engineering_add_test(C1_odr_student
        COMMAND C1_odr_student
        LABELS student
        TIMEOUT 30
    )
endif()

engineering_add_test(C1_odr_observation_single
    COMMAND C1_odr_observation_single
    LABELS observation
    TIMEOUT 30
)

engineering_add_test(C1_odr_observation_preprocess
    COMMAND
        ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/observe_preprocess.cmake
    LABELS observation
    TIMEOUT 30
)

get_filename_component(_c1_odr_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
find_program(C1_ODR_DUMPBIN dumpbin HINTS "${_c1_odr_compiler_dir}")
find_program(C1_ODR_NM nm)


set(_c1_odr_generator_args -DGENERATOR=${CMAKE_GENERATOR})
if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND _c1_odr_generator_args -DPLATFORM_ARG=${CMAKE_GENERATOR_PLATFORM})
endif()
if(CMAKE_GENERATOR MATCHES "Ninja" AND CMAKE_MAKE_PROGRAM)
    list(APPEND _c1_odr_generator_args -DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM})
endif()

engineering_add_test(C1_odr_observation_symbols
    COMMAND
        ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DGENERATOR=${CMAKE_GENERATOR}
        -DBUILD_ROOT=${CMAKE_BINARY_DIR}
        -DCONFIG=$<CONFIG>
        -DDUMPBIN=${C1_ODR_DUMPBIN}
        -DNM=${C1_ODR_NM}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/observe_symbols.cmake
    LABELS observation
    TIMEOUT 30
)

engineering_add_test(C1_odr_negative_duplicate_header_definition
    COMMAND
        ${CMAKE_COMMAND}
        -DCASE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/negative/duplicate_header_definition
        -DCASE_BUILD=${CMAKE_CURRENT_BINARY_DIR}/negative/duplicate_header_definition
        -DOBJECT_TARGET=C1_odr_negative_duplicate_header_definition_objects
        -DLINK_TARGET=C1_odr_negative_duplicate_header_definition_link
        ${_c1_odr_generator_args}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_build_failure.cmake
    LABELS negative
    TIMEOUT 180
)

engineering_add_test(C1_odr_negative_include_guard_same_failure
    COMMAND
        ${CMAKE_COMMAND}
        -DCASE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/negative/include_guard_same_failure
        -DCASE_BUILD=${CMAKE_CURRENT_BINARY_DIR}/negative/include_guard_same_failure
        -DOBJECT_TARGET=C1_odr_negative_include_guard_same_failure_objects
        -DLINK_TARGET=C1_odr_negative_include_guard_same_failure_link
        ${_c1_odr_generator_args}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_build_failure.cmake
    LABELS negative
    TIMEOUT 180
)

engineering_setup_ide(C1_odr_student
    FILES
        cmake/expect_build_failure.cmake
        cmake/observe_preprocess.cmake
        cmake/observe_symbols.cmake
        negative/duplicate_header_definition/CMakeLists.txt
        negative/include_guard_same_failure/CMakeLists.txt
        review_variants/negative_compile/CMakeLists.txt
        review_variants/negative_missing_symbol/CMakeLists.txt
        review_variants/student_bypass/CMakeLists.txt
        review_variants/symbol_missing/CMakeLists.txt
    BROWSE
        negative/duplicate_header_definition/caller_a.cpp
        negative/duplicate_header_definition/caller_b.cpp
        negative/duplicate_header_definition/lesson.hpp
        negative/duplicate_header_definition/main.cpp
        negative/include_guard_same_failure/caller_a.cpp
        negative/include_guard_same_failure/caller_b.cpp
        negative/include_guard_same_failure/lesson.hpp
        negative/include_guard_same_failure/main.cpp
        review_variants/negative_compile/caller_a.cpp
        review_variants/negative_compile/caller_b.cpp
        review_variants/negative_compile/lesson.hpp
        review_variants/negative_compile/main.cpp
        review_variants/negative_missing_symbol/caller_a.cpp
        review_variants/negative_missing_symbol/caller_b.cpp
        review_variants/negative_missing_symbol/lesson.hpp
        review_variants/negative_missing_symbol/main.cpp
        review_variants/student_bypass/caller_a.cpp
        review_variants/student_bypass/caller_b.cpp
        review_variants/student_bypass/lesson.cpp
        review_variants/student_bypass/lesson.hpp
        review_variants/symbol_missing/lesson.hpp
        review_variants/symbol_missing/observer_a.cpp
        review_variants/symbol_missing/observer_b.cpp
)
