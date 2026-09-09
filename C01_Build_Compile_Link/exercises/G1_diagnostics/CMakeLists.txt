cmake_minimum_required(VERSION 3.28)
project(G1_diagnostics LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

add_executable(G1_diagnostics_reference checks/reference_check.cpp reference/parser.cpp)
target_include_directories(G1_diagnostics_reference PRIVATE reference)
engineering_configure_target(G1_diagnostics_reference)

add_executable(G1_diagnostics_student checks/student_check.cpp src/student/parser.cpp)
target_include_directories(G1_diagnostics_student PRIVATE src/student)
engineering_configure_target(G1_diagnostics_student)

engineering_add_test(G1_diagnostics_reference
    COMMAND G1_diagnostics_reference
    LABELS reference
    TIMEOUT 30
)

if(ENGINEERING_STUDY_TEST_STUDENTS)
    engineering_add_test(G1_diagnostics_student
        COMMAND G1_diagnostics_student
        LABELS student
        TIMEOUT 30
    )
endif()

if(ENGINEERING_STUDY_ENABLE_UNSAFE_DEMOS)
    add_executable(G1_diagnostics_asan_fault unsafe/asan_fault.cpp)
    engineering_configure_target(G1_diagnostics_asan_fault)
endif()
