cmake_minimum_required(VERSION 3.28)
project(L08_unique LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

option(L08_UNIQUE_BUILD_VALIDATION_VARIANTS "Build local checker good/bad validation variants" OFF)

add_executable(L08_unique_observation checks/observation_check.cpp)
target_include_directories(L08_unique_observation PRIVATE checks checks/support)
core_configure_target(L08_unique_observation)
core_add_test(L08_unique_observation COMMAND L08_unique_observation LABELS observation TIMEOUT 30)

if(CORE_STUDY_TEST_STUDENTS)
    add_executable(L08_unique_student checks/student_check.cpp)
else()
    add_executable(L08_unique_student EXCLUDE_FROM_ALL checks/student_check.cpp)
endif()
target_include_directories(L08_unique_student PRIVATE checks/support src/student checks)
core_configure_target(L08_unique_student)
if(CORE_STUDY_TEST_STUDENTS)
    core_add_test(L08_unique_student COMMAND L08_unique_student LABELS student TIMEOUT 30)
endif()

if(CORE_STUDY_BUILD_REFERENCE)
    add_executable(L08_unique_reference checks/reference_check.cpp)
    target_include_directories(L08_unique_reference PRIVATE checks/support src/reference checks)
    core_configure_target(L08_unique_reference)
    core_add_test(L08_unique_reference COMMAND L08_unique_reference LABELS reference TIMEOUT 30)
endif()

if(L08_UNIQUE_BUILD_VALIDATION_VARIANTS)
    add_executable(L08_unique_validation_good checks/validation_check.cpp)
    target_include_directories(L08_unique_validation_good PRIVATE checks/support validation/good checks)
    core_configure_target(L08_unique_validation_good)

    add_executable(L08_unique_validation_bad_noop checks/validation_check.cpp)
    target_include_directories(L08_unique_validation_bad_noop PRIVATE checks/support validation/bad_noop checks)
    core_configure_target(L08_unique_validation_bad_noop)

    add_executable(L08_unique_validation_bad_release_keeps_owner checks/validation_check.cpp)
    target_include_directories(L08_unique_validation_bad_release_keeps_owner PRIVATE checks/support validation/bad_release_keeps_owner checks)
    core_configure_target(L08_unique_validation_bad_release_keeps_owner)
endif()

