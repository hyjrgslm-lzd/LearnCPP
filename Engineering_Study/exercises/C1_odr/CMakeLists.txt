cmake_minimum_required(VERSION 3.28)

project(C1_odr LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

add_library(C1_odr_student_impl
    support/student_callers/caller_a.cpp
    support/student_callers/caller_b.cpp
    src/student/lesson.cpp
)
target_include_directories(C1_odr_student_impl PUBLIC src/student)
engineering_configure_target(C1_odr_student_impl)

add_executable(C1_odr_student
    checks/student_check.cpp
)
target_link_libraries(C1_odr_student PRIVATE C1_odr_student_impl)
engineering_configure_target(C1_odr_student)

if(ENGINEERING_STUDY_BUILD_REFERENCE)
    add_library(C1_odr_reference_impl
        reference/split_definition/caller_a.cpp
        reference/split_definition/caller_b.cpp
        reference/split_definition/lesson.cpp
    )
    target_include_directories(C1_odr_reference_impl PUBLIC reference/split_definition)
    engineering_configure_target(C1_odr_reference_impl)

    add_executable(C1_odr_reference
        checks/reference_check.cpp
    )
    target_link_libraries(C1_odr_reference PRIVATE C1_odr_reference_impl)
    engineering_configure_target(C1_odr_reference)

    add_executable(C1_odr_reference_inline
        reference/inline_definition/main.cpp
        reference/inline_definition/caller_a.cpp
        reference/inline_definition/caller_b.cpp
    )
    target_include_directories(C1_odr_reference_inline PRIVATE reference/inline_definition)
    engineering_configure_target(C1_odr_reference_inline)
endif()

add_executable(C1_odr_observation_single
    observation/single_consumer/main.cpp
)
target_include_directories(C1_odr_observation_single PRIVATE observation/single_consumer)
engineering_configure_target(C1_odr_observation_single)

add_library(C1_odr_observation_objects OBJECT
    observation/two_tu_symbols/observer_a.cpp
    observation/two_tu_symbols/observer_b.cpp
)
target_include_directories(C1_odr_observation_objects PRIVATE observation/two_tu_symbols)
engineering_configure_target(C1_odr_observation_objects)

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
