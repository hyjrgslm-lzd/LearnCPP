cmake_minimum_required(VERSION 3.28)
project(A1_build_debug LANGUAGES CXX)
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

add_library(A1_build_debug_reference_impl src/reference/debug_story.cpp)
target_include_directories(A1_build_debug_reference_impl PUBLIC src/reference)
engineering_configure_target(A1_build_debug_reference_impl)

add_executable(A1_build_debug_reference checks/reference_check.cpp)
target_link_libraries(A1_build_debug_reference PRIVATE A1_build_debug_reference_impl)
engineering_configure_target(A1_build_debug_reference)

add_executable(A1_build_debug_observation checks/observation_check.cpp)
target_compile_definitions(A1_build_debug_observation PRIVATE
    A1_CONFIG_NAME="$<CONFIG>"
    A1_EXPECT_NDEBUG=$<IF:$<CONFIG:Release>,1,0>
    A1_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
engineering_configure_target(A1_build_debug_observation)

add_library(A1_build_debug_student_impl src/student/debug_story.cpp)
target_include_directories(A1_build_debug_student_impl PUBLIC src/student)
engineering_configure_target(A1_build_debug_student_impl)

add_executable(A1_build_debug_student checks/student_check.cpp)
target_link_libraries(A1_build_debug_student PRIVATE A1_build_debug_student_impl)
engineering_configure_target(A1_build_debug_student)

engineering_add_test(A1_build_debug_reference COMMAND A1_build_debug_reference LABELS reference TIMEOUT 30)
engineering_add_test(A1_build_debug_observation COMMAND A1_build_debug_observation LABELS observation TIMEOUT 30)
if(ENGINEERING_STUDY_TEST_STUDENTS)
    engineering_add_test(A1_build_debug_student COMMAND A1_build_debug_student LABELS student TIMEOUT 30)
endif()

