cmake_minimum_required(VERSION 3.28)
project(L03_lifetimes LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

function(l03_add_executable target impl_dir source)
    add_executable(${target} checks/lifetime_checks.cpp ${source})
    target_include_directories(${target} PRIVATE checks checks/support ${impl_dir})
    target_compile_definitions(${target} PRIVATE L03_IMPLEMENTATION_HEADER=\"borrowed_int.hpp\")
    core_configure_target(${target})
endfunction()

function(l03_add_case target impl_dir source labels)
    l03_add_executable(${target} ${impl_dir} ${source})
    core_add_test(${target} COMMAND ${target} LABELS ${labels} TIMEOUT 30)
endfunction()

function(l03_expected_failure test_name target expected_text)
    add_test(NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DEXECUTABLE=$<TARGET_FILE:${target}>
            -DEXPECTED_TEXT=${expected_text}
            -P ${CMAKE_CURRENT_LIST_DIR}/checks/expect_failure.cmake
    )
    set_tests_properties(${test_name} PROPERTIES LABELS negative TIMEOUT 30)
endfunction()

l03_add_executable(L03_lifetimes_student "${CMAKE_CURRENT_LIST_DIR}/src/student" src/student/borrowed_int.cpp)
if(CORE_STUDY_TEST_STUDENTS)
    core_add_test(L03_lifetimes_student COMMAND L03_lifetimes_student LABELS student TIMEOUT 30)
endif()

if(CORE_STUDY_BUILD_REFERENCE)
    l03_add_case(L03_lifetimes_reference "${CMAKE_CURRENT_LIST_DIR}/reference" reference/borrowed_int.cpp reference)
    l03_add_case(L03_lifetimes_validation_good "${CMAKE_CURRENT_LIST_DIR}/validation/good" validation/good/borrowed_int.cpp validation)

    l03_add_executable(L03_lifetimes_validation_bad_dangling_view
        "${CMAKE_CURRENT_LIST_DIR}/validation/bad_dangling_view"
        validation/bad_dangling_view/borrowed_int.cpp)
    if(BUILD_TESTING)
        l03_expected_failure(L03_lifetimes_validation_bad_dangling_view_check
            L03_lifetimes_validation_bad_dangling_view
            "check failed: borrow must become invalid when owner lifetime ends")
    endif()
endif()
