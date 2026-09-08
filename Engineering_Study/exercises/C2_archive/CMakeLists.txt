cmake_minimum_required(VERSION 3.28)
project(C2_archive LANGUAGES CXX)
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

add_library(C2_archive_library STATIC
    src/reference/used_member.cpp
    src/reference/unused_member.cpp
)
target_include_directories(C2_archive_library PUBLIC src/reference)
engineering_configure_target(C2_archive_library)

add_executable(C2_archive_reference checks/reference_check.cpp)
target_link_libraries(C2_archive_reference PRIVATE C2_archive_library)
engineering_configure_target(C2_archive_reference)

add_library(C2_archive_student_impl src/student/archive_value.cpp)
target_include_directories(C2_archive_student_impl PUBLIC src/student)
engineering_configure_target(C2_archive_student_impl)

add_executable(C2_archive_student checks/student_check.cpp)
target_link_libraries(C2_archive_student PRIVATE C2_archive_student_impl)
engineering_configure_target(C2_archive_student)

get_filename_component(_c2_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
find_program(C2_DUMPBIN dumpbin HINTS "${_c2_compiler_dir}")
find_program(C2_NM nm)

set(_c2_generator_args -DGENERATOR=${CMAKE_GENERATOR})
if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND _c2_generator_args -DPLATFORM_ARG=${CMAKE_GENERATOR_PLATFORM})
endif()
if(CMAKE_GENERATOR MATCHES "Ninja" AND CMAKE_MAKE_PROGRAM)
    list(APPEND _c2_generator_args -DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM})
endif()

engineering_add_test(C2_archive_reference COMMAND C2_archive_reference LABELS reference TIMEOUT 30)
engineering_add_test(C2_archive_symbols
    COMMAND ${CMAKE_COMMAND}
    -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DBUILD_ROOT=${CMAKE_BINARY_DIR}
        -DCONFIG=$<CONFIG>
        -DDUMPBIN=${C2_DUMPBIN}
        -DNM=${C2_NM}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_symbols.cmake
    LABELS observation
    TIMEOUT 30
)
engineering_add_test(C2_archive_negative_missing_definition
    COMMAND ${CMAKE_COMMAND}
        -DCASE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/negative/missing_definition
        -DCASE_BUILD=${CMAKE_CURRENT_BINARY_DIR}/negative/missing_definition
        -DOBJECT_TARGET=C2_archive_negative_missing_definition_objects
        -DLINK_TARGET=C2_archive_negative_missing_definition_link
        -DSYMBOL_NAME=missing_archive_value
        ${_c2_generator_args}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_link_failure.cmake
    LABELS negative
    TIMEOUT 180
)
engineering_add_test(C2_archive_negative_direct_objects
    COMMAND ${CMAKE_COMMAND}
        -DCASE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/negative/direct_objects
        -DCASE_BUILD=${CMAKE_CURRENT_BINARY_DIR}/negative/direct_objects
        -DOBJECT_TARGET=C2_archive_negative_direct_objects
        -DLINK_TARGET=C2_archive_negative_direct_objects_link
        -DSYMBOL_NAME=missing_unused_dependency
        ${_c2_generator_args}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_link_failure.cmake
    LABELS negative
    TIMEOUT 180
)
if(ENGINEERING_STUDY_TEST_STUDENTS)
    engineering_add_test(C2_archive_student COMMAND C2_archive_student LABELS student TIMEOUT 30)
endif()
