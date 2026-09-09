cmake_minimum_required(VERSION 3.28)
project(L05_special_members LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

function(l05_add_executable target impl_dir source)
    add_executable(${target} checks/buffer_checks.cpp ${source})
    target_include_directories(${target} PRIVATE checks checks/support ${impl_dir})
    target_compile_definitions(${target} PRIVATE L05_IMPLEMENTATION_HEADER=\"int_buffer.hpp\")
    core_configure_target(${target})
endfunction()

function(l05_add_case target impl_dir source labels)
    l05_add_executable(${target} ${impl_dir} ${source})
    core_add_test(${target} COMMAND ${target} LABELS ${labels} TIMEOUT 30)
endfunction()

function(l05_expected_failure test_name target expected_text)
    add_test(NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DEXECUTABLE=$<TARGET_FILE:${target}>
            -DEXPECTED_TEXT=${expected_text}
            -P ${CMAKE_CURRENT_LIST_DIR}/checks/expect_failure.cmake
    )
    set_tests_properties(${test_name} PROPERTIES LABELS negative TIMEOUT 30)
endfunction()

l05_add_executable(L05_special_members_student "${CMAKE_CURRENT_LIST_DIR}/src/student" src/student/int_buffer.cpp)
if(CORE_STUDY_TEST_STUDENTS)
    core_add_test(L05_special_members_student COMMAND L05_special_members_student LABELS student TIMEOUT 30)
endif()

if(CORE_STUDY_BUILD_REFERENCE)
    l05_add_case(L05_special_members_reference "${CMAKE_CURRENT_LIST_DIR}/reference" reference/int_buffer.cpp reference)
    l05_add_case(L05_special_members_validation_good "${CMAKE_CURRENT_LIST_DIR}/validation/good" validation/good/int_buffer.cpp validation)

    l05_add_executable(L05_special_members_validation_bad_shallow_copy
        "${CMAKE_CURRENT_LIST_DIR}/validation/bad_shallow_copy"
        validation/bad_shallow_copy/int_buffer.cpp)
    if(BUILD_TESTING)
        l05_expected_failure(L05_special_members_validation_bad_shallow_copy_check
            L05_special_members_validation_bad_shallow_copy
            "check failed: copy must have independent storage")
    endif()

    l05_add_executable(L05_special_members_validation_bad_move_leak
        "${CMAKE_CURRENT_LIST_DIR}/validation/bad_move_leak"
        validation/bad_move_leak/int_buffer.cpp)
    if(BUILD_TESTING)
        l05_expected_failure(L05_special_members_validation_bad_move_leak_check
            L05_special_members_validation_bad_move_leak
            "check failed: move assignment should release old target storage")
    endif()
endif()
