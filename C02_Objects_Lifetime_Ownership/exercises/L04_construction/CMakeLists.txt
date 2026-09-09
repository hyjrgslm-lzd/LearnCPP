cmake_minimum_required(VERSION 3.28)

project(L04_construction LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

function(l04_add_executable target impl_dir impl_source)
    add_executable(${target}
        checks/run_checks.cpp
        ${impl_source}
    )
    target_include_directories(${target} PRIVATE
        checks
        ${impl_dir}
    )
    target_compile_definitions(${target} PRIVATE
        L04_IMPLEMENTATION_HEADER=\"construction_lab.hpp\"
    )
    core_configure_target(${target})
endfunction()

function(l04_add_case target impl_dir impl_source labels)
    l04_add_executable(${target} ${impl_dir} ${impl_source})
    core_add_test(${target}
        COMMAND ${target}
        LABELS ${labels}
        TIMEOUT 30
    )
endfunction()

function(l04_add_expected_failure test_name target expected_text)
    if(NOT BUILD_TESTING)
        return()
    endif()
    add_test(NAME ${test_name}
        COMMAND
            ${CMAKE_COMMAND}
            -DEXECUTABLE=$<TARGET_FILE:${target}>
            -DEXPECTED_TEXT=${expected_text}
            -P ${CMAKE_CURRENT_LIST_DIR}/checks/expect_failure.cmake
    )
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 30 LABELS negative)
endfunction()

l04_add_executable(L04_construction_student
    "${CMAKE_CURRENT_LIST_DIR}/src/student"
    src/student/construction_lab.cpp
)
if(CORE_STUDY_TEST_STUDENTS AND BUILD_TESTING)
    core_add_test(L04_construction_student
        COMMAND L04_construction_student
        LABELS student
        TIMEOUT 30
    )
endif()

if(CORE_STUDY_BUILD_REFERENCE)
    l04_add_case(L04_construction_reference
        "${CMAKE_CURRENT_LIST_DIR}/reference"
        reference/construction_lab.cpp
        "reference"
    )

    l04_add_case(L04_construction_validation_good
        "${CMAKE_CURRENT_LIST_DIR}/validation/good"
        validation/good/construction_lab.cpp
        "validation"
    )

    add_executable(L04_construction_validation_bad_noop
        checks/run_checks.cpp
        validation/bad_noop/construction_lab.cpp
    )
    target_include_directories(L04_construction_validation_bad_noop PRIVATE
        checks
        "${CMAKE_CURRENT_LIST_DIR}/validation/bad_noop"
    )
    target_compile_definitions(L04_construction_validation_bad_noop PRIVATE
        L04_IMPLEMENTATION_HEADER=\"construction_lab.hpp\"
    )
    core_configure_target(L04_construction_validation_bad_noop)
    l04_add_expected_failure(L04_construction_validation_bad_noop_check
        L04_construction_validation_bad_noop
        "check failed: both resources should be live inside the complete object scope"
    )

    add_executable(L04_construction_validation_bad_leak
        checks/run_checks.cpp
        validation/bad_leak/construction_lab.cpp
    )
    target_include_directories(L04_construction_validation_bad_leak PRIVATE
        checks
        "${CMAKE_CURRENT_LIST_DIR}/validation/bad_leak"
    )
    target_compile_definitions(L04_construction_validation_bad_leak PRIVATE
        L04_IMPLEMENTATION_HEADER=\"construction_lab.hpp\"
    )
    core_configure_target(L04_construction_validation_bad_leak)
    l04_add_expected_failure(L04_construction_validation_bad_leak_check
        L04_construction_validation_bad_leak
        "check failed: completed first member should release during unwinding"
    )
endif()
