cmake_minimum_required(VERSION 3.28)
project(L14_ub LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

add_executable(L14_ub_observation checks/observation_check.cpp)
core_configure_target(L14_ub_observation)
core_add_test(L14_ub_observation COMMAND L14_ub_observation LABELS observation TIMEOUT 30)

function(l14_frontier_probe target source)
    add_library(${target} OBJECT EXCLUDE_FROM_ALL ${source})
    core_configure_target(${target})
    if(MSVC)
        target_compile_options(${target} PRIVATE /std:c++latest)
    else()
        target_compile_options(${target} PRIVATE -std=c++2c)
    endif()
endfunction()

function(l14_frontier_test test_name target expectation source_marker expected_failure accepted_diagnostic)
    add_test(NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DBUILD_DIR=${CMAKE_BINARY_DIR}
            -DCONFIG=$<CONFIG>
            -DTARGET=${target}
            -DCASE=${test_name}
            -DEXPECT=${expectation}
            -DSOURCE_MARKER=${source_marker}
            -DEXPECTED_FAILURE=${expected_failure}
            -DACCEPTED_DIAGNOSTIC=${accepted_diagnostic}
            -DOUT_DIR=${CMAKE_BINARY_DIR}/frontier
            -P ${CMAKE_CURRENT_LIST_DIR}/checks/frontier_compile.cmake)
    set_tests_properties(${test_name}
        PROPERTIES
            TIMEOUT 90
            LABELS frontier
            RESOURCE_LOCK L14_frontier_build_tree
            SKIP_REGULAR_EXPRESSION "SKIP:")
endfunction()

if(CORE_STUDY_ENABLE_FRONTIER)
    l14_frontier_probe(L14_frontier_baseline checks/frontier/baseline.cpp)
    l14_frontier_test(L14_frontier_baseline_compile
        L14_frontier_baseline
        must_pass
        "baseline.cpp"
        ""
        "")

    l14_frontier_probe(L14_frontier_p2287_base_member_designator
        checks/frontier/p2287_base_member_designator.cpp)
    l14_frontier_test(L14_frontier_p2287_base_member_designator_compile
        L14_frontier_p2287_base_member_designator
        capability_compile
        "p2287_base_member_designator.cpp"
        "C7559|C2440:.*nitializer list.*erived|field designator.*does not refer|designator.*direct.*member"
        "")

    l14_frontier_probe(L14_frontier_p2748_return_temp_ref
        checks/frontier/p2748_return_temp_ref.cpp)
    l14_frontier_test(L14_frontier_p2748_return_temp_ref_compile
        L14_frontier_p2748_return_temp_ref
        capability_reject
        "p2748_return_temp_ref.cpp"
        "returning reference to local temporary|return.*temporary|C4172"
        "warning C4172|returning reference to local temporary|return.*temporary")

    l14_frontier_probe(L14_frontier_p2953_defaulted_assignment
        checks/frontier/p2953_defaulted_assignment.cpp)
    l14_frontier_test(L14_frontier_p2953_defaulted_assignment_compile
        L14_frontier_p2953_defaulted_assignment
        capability_reject
        "p2953_defaulted_assignment.cpp"
        "defaulted.*(ref-qualifier|rvalue)|ref-qualifier.*defaulted|rvalue.*defaulted"
        "defaulted.*(ref-qualifier|rvalue)|ref-qualifier.*defaulted|rvalue.*defaulted")

    l14_frontier_probe(L14_frontier_provenance_lifetime_end_model
        checks/frontier/provenance_lifetime_end_model.cpp)
    l14_frontier_test(L14_frontier_provenance_lifetime_end_model_compile
        L14_frontier_provenance_lifetime_end_model
        observe_compile
        "provenance_lifetime_end_model.cpp"
        ""
        "")
    set_tests_properties(L14_frontier_baseline_compile PROPERTIES
        FIXTURES_SETUP L14_frontier_baseline_ready)
    set_tests_properties(
        L14_frontier_p2287_base_member_designator_compile
        L14_frontier_p2748_return_temp_ref_compile
        L14_frontier_p2953_defaulted_assignment_compile
        L14_frontier_provenance_lifetime_end_model_compile
        PROPERTIES FIXTURES_REQUIRED L14_frontier_baseline_ready)
endif()

if(CORE_STUDY_ENABLE_ASAN AND CORE_STUDY_ENABLE_UNSAFE_DEMOS)
    add_executable(L14_ub_asan_uaf checks/asan_uaf_probe.cpp)
    core_configure_target(L14_ub_asan_uaf)
    add_test(NAME L14_ub_asan_uaf_diagnostic
        COMMAND ${CMAKE_COMMAND}
            -DEXECUTABLE=$<TARGET_FILE:L14_ub_asan_uaf>
            -DEXPECTED_TEXT=heap-use-after-free
            -DEXPECTED_SOURCE=asan_uaf_probe.cpp
            -P ${CMAKE_CURRENT_LIST_DIR}/checks/expect_asan_failure.cmake)
    set_tests_properties(L14_ub_asan_uaf_diagnostic PROPERTIES TIMEOUT 30 LABELS "unsafe;asan")
endif()
