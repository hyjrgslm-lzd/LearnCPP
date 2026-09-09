cmake_minimum_required(VERSION 3.28)
project(P1_object_buffer LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

function(p1_configure target include_dir)
    add_executable(${target} checks/run_checks.cpp)
    target_include_directories(${target} PRIVATE checks ${include_dir})
    core_configure_target(${target})
endfunction()

function(p1_expected_failure test_name target expected_text)
    add_test(NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DEXECUTABLE=$<TARGET_FILE:${target}>
            -DEXPECTED_TEXT=${expected_text}
            -P ${CMAKE_CURRENT_LIST_DIR}/checks/expect_failure.cmake)
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 30 LABELS negative)
endfunction()

if(CORE_STUDY_TEST_STUDENTS)
    p1_configure(P1_object_buffer_student src/student)
    core_add_test(P1_object_buffer_student COMMAND P1_object_buffer_student LABELS student TIMEOUT 30)
else()
    add_executable(P1_object_buffer_student EXCLUDE_FROM_ALL checks/run_checks.cpp)
    target_include_directories(P1_object_buffer_student PRIVATE checks src/student)
    core_configure_target(P1_object_buffer_student)
endif()

if(CORE_STUDY_BUILD_REFERENCE)
    p1_configure(P1_object_buffer_reference src/reference)
    core_add_test(P1_object_buffer_reference COMMAND P1_object_buffer_reference LABELS reference TIMEOUT 30)

    p1_configure(P1_object_buffer_validation_good validation/good)
    core_add_test(P1_object_buffer_validation_good COMMAND P1_object_buffer_validation_good LABELS validation TIMEOUT 30)

    p1_configure(P1_object_buffer_validation_bad_noop validation/bad_noop)
    p1_expected_failure(P1_object_buffer_validation_bad_noop_check
        P1_object_buffer_validation_bad_noop
        "reserve grows capacity")

    p1_configure(P1_object_buffer_validation_bad_early_commit validation/bad_early_commit)
    p1_expected_failure(P1_object_buffer_validation_bad_early_commit_check
        P1_object_buffer_validation_bad_early_commit
        "new tail failure keeps capacity")

    add_executable(P1_object_buffer_reject_throwing_move_only EXCLUDE_FROM_ALL checks/reject_throwing_move_only.cpp)
    target_include_directories(P1_object_buffer_reject_throwing_move_only PRIVATE src/reference)
    core_configure_target(P1_object_buffer_reject_throwing_move_only)
    add_test(NAME P1_object_buffer_reject_throwing_move_only_check
        COMMAND ${CMAKE_COMMAND}
            -DBUILD_DIR=${CMAKE_BINARY_DIR}
            -DCONFIG=$<CONFIG>
            -DTARGET=P1_object_buffer_reject_throwing_move_only
            -P ${CMAKE_CURRENT_LIST_DIR}/checks/expect_compile_failure.cmake)
    set_tests_properties(P1_object_buffer_reject_throwing_move_only_check PROPERTIES TIMEOUT 30 LABELS negative)
endif()
