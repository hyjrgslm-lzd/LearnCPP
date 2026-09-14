cmake_minimum_required(VERSION 3.28)
project(B1_preprocessor LANGUAGES CXX)
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

if(ENGINEERING_STUDY_BUILD_REFERENCE)
    add_executable(B1_preprocessor_reference
        checks/reference_check.cpp
        src/reference/macro_a.cpp
        src/reference/macro_b.cpp
    )
    target_include_directories(B1_preprocessor_reference PRIVATE src/reference)
    engineering_configure_target(B1_preprocessor_reference)
    target_sources(B1_preprocessor_reference PRIVATE src/reference/generated_value.hpp src/reference/pragma_once_example.hpp)
endif()

add_executable(B1_preprocessor_student
    checks/student_check.cpp
    src/student/student_value.cpp
)
target_include_directories(B1_preprocessor_student PRIVATE src/student)
engineering_configure_target(B1_preprocessor_student)
target_sources(B1_preprocessor_student PRIVATE src/student/student_value.hpp)

if(ENGINEERING_STUDY_BUILD_REFERENCE)
    engineering_add_test(B1_preprocessor_reference COMMAND B1_preprocessor_reference LABELS reference TIMEOUT 30)
endif()
engineering_add_test(B1_preprocessor_preprocess
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_preprocess.cmake
    LABELS observation
    TIMEOUT 30
)
if(ENGINEERING_STUDY_TEST_STUDENTS)
    engineering_add_test(B1_preprocessor_student COMMAND B1_preprocessor_student LABELS student TIMEOUT 30)
endif()

engineering_setup_ide(B1_preprocessor_student
    FILES
        cmake/check_preprocess.cmake
)
