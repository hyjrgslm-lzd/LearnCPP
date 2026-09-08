cmake_minimum_required(VERSION 3.28)
project(F1_cmake_targets LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

add_library(F1_warning_policy INTERFACE)
target_compile_definitions(F1_warning_policy INTERFACE F1_REQUIRE_EXPLICIT_RESULT=1)

add_library(F1_math_public reference/math.cpp)
target_include_directories(F1_math_public PUBLIC reference/include)
target_link_libraries(F1_math_public PUBLIC F1_warning_policy)
target_compile_definitions(F1_math_public PRIVATE F1_BUILDING_MATH=1)
engineering_configure_target(F1_math_public)

add_executable(F1_cmake_targets_reference checks/reference_check.cpp)
target_link_libraries(F1_cmake_targets_reference PRIVATE F1_math_public)
engineering_configure_target(F1_cmake_targets_reference)

add_library(F1_student_impl src/student/student.cpp)
target_include_directories(F1_student_impl PUBLIC src/student)
target_link_libraries(F1_student_impl PUBLIC F1_warning_policy)
engineering_configure_target(F1_student_impl)

add_executable(F1_cmake_targets_student checks/student_check.cpp)
target_link_libraries(F1_cmake_targets_student PRIVATE F1_student_impl)
engineering_configure_target(F1_cmake_targets_student)

engineering_add_test(F1_cmake_targets_reference
    COMMAND F1_cmake_targets_reference
    LABELS reference
    TIMEOUT 30
)

if(ENGINEERING_STUDY_TEST_STUDENTS)
    engineering_add_test(F1_cmake_targets_student
        COMMAND F1_cmake_targets_student
        LABELS student
        TIMEOUT 30
    )
endif()
