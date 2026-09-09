cmake_minimum_required(VERSION 3.28)

project(C1_odr_review_student_bypass LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/StudySetup.cmake")

add_library(C1_odr_student_impl
    caller_a.cpp
    caller_b.cpp
    lesson.cpp
)
target_include_directories(C1_odr_student_impl PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
engineering_configure_target(C1_odr_student_impl)

add_executable(C1_odr_student
    ../../checks/student_check.cpp
)
target_link_libraries(C1_odr_student PRIVATE C1_odr_student_impl)
engineering_configure_target(C1_odr_student)

engineering_add_test(C1_odr_student
    COMMAND C1_odr_student
    LABELS review-negative
    TIMEOUT 30
)

