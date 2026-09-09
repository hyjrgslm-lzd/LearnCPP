cmake_minimum_required(VERSION 3.28)
project(L10_control_block LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

option(L10_CONTROL_BLOCK_BUILD_VALIDATION_VARIANTS "Build local checker good/bad validation variants" OFF)

add_executable(L10_control_block_observation checks/observation_check.cpp)
target_include_directories(L10_control_block_observation PRIVATE checks checks/support)
core_configure_target(L10_control_block_observation)
core_add_test(L10_control_block_observation COMMAND L10_control_block_observation LABELS observation TIMEOUT 30)

if(CORE_STUDY_TEST_STUDENTS)
    add_executable(L10_control_block_student checks/student_check.cpp)
else()
    add_executable(L10_control_block_student EXCLUDE_FROM_ALL checks/student_check.cpp)
endif()
target_include_directories(L10_control_block_student PRIVATE checks/support src/student checks)
core_configure_target(L10_control_block_student)
if(CORE_STUDY_TEST_STUDENTS)
    core_add_test(L10_control_block_student COMMAND L10_control_block_student LABELS student TIMEOUT 30)
endif()

if(CORE_STUDY_BUILD_REFERENCE)
    add_executable(L10_control_block_reference checks/reference_check.cpp)
    target_include_directories(L10_control_block_reference PRIVATE checks/support src/reference checks)
    core_configure_target(L10_control_block_reference)
    core_add_test(L10_control_block_reference COMMAND L10_control_block_reference LABELS reference TIMEOUT 30)
endif()

if(L10_CONTROL_BLOCK_BUILD_VALIDATION_VARIANTS)
    add_executable(L10_control_block_validation_good checks/validation_check.cpp)
    target_include_directories(L10_control_block_validation_good PRIVATE checks/support validation/good checks)
    core_configure_target(L10_control_block_validation_good)

    add_executable(L10_control_block_validation_bad_leak_control_block checks/validation_check.cpp)
    target_include_directories(L10_control_block_validation_bad_leak_control_block PRIVATE checks/support validation/bad_leak_control_block checks)
    core_configure_target(L10_control_block_validation_bad_leak_control_block)

    add_executable(L10_control_block_validation_bad_weak_resurrect checks/validation_check.cpp)
    target_include_directories(L10_control_block_validation_bad_weak_resurrect PRIVATE checks/support validation/bad_weak_resurrect checks)
    core_configure_target(L10_control_block_validation_bad_weak_resurrect)
endif()

