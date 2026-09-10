cmake_minimum_required(VERSION 3.28)
project(U01_async_logging LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
include(../cmake/SpdlogSetup.cmake)

if(NOT CONCURRENCY_STUDY_ENABLE_SPDLOG)
    message(STATUS "U01_async_logging disabled; CONCURRENCY_STUDY_ENABLE_SPDLOG=OFF")
    return()
endif()

cs_add_exercise(U01_async_logging IMPLEMENTATION)
target_include_directories(U01_async_logging PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src/student")
cs_link_spdlog(U01_async_logging)

if(CONCURRENCY_STUDY_BUILD_REFERENCE)
    target_include_directories(U01_async_logging_reference PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src/reference")
    cs_link_spdlog(U01_async_logging_reference)

    foreach(case IN ITEMS good bad_overflow bad_flush)
        set(target "U01_async_logging_${case}")
        add_executable(${target} check_main.cpp)
        cs_configure_target(${target})
        target_include_directories(${target} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/validation/${case}")
        cs_link_spdlog(${target})
    endforeach()

    add_custom_target(U01_async_logging_checked DEPENDS
        U01_async_logging_reference U01_async_logging_good
        U01_async_logging_bad_overflow U01_async_logging_bad_flush)

    if(BUILD_TESTING)
        add_test(NAME U01_async_logging_good COMMAND U01_async_logging_good)
        math(EXPR CS_U01_EXPECT_FAILURE_CTEST_TIMEOUT "${CS_TEST_TIMEOUT} + 5")
        add_test(NAME U01_async_logging_bad_overflow_rejected COMMAND ${CMAKE_COMMAND}
            "-DEXECUTABLE=$<TARGET_FILE:U01_async_logging_bad_overflow>"
            "-DEXPECTED_TEXT=overrun counter"
            "-DPROCESS_TIMEOUT=${CS_TEST_TIMEOUT}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/expect_failure.cmake")
        add_test(NAME U01_async_logging_bad_flush_rejected COMMAND ${CMAKE_COMMAND}
            "-DEXECUTABLE=$<TARGET_FILE:U01_async_logging_bad_flush>"
            "-DEXPECTED_TEXT=queued flush runs during drain"
            "-DPROCESS_TIMEOUT=${CS_TEST_TIMEOUT}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/expect_failure.cmake")
        set_tests_properties(U01_async_logging_good PROPERTIES TIMEOUT ${CS_TEST_TIMEOUT} LABELS validation)
        set_tests_properties(U01_async_logging_bad_overflow_rejected PROPERTIES TIMEOUT ${CS_U01_EXPECT_FAILURE_CTEST_TIMEOUT} LABELS negative)
        set_tests_properties(U01_async_logging_bad_flush_rejected PROPERTIES TIMEOUT ${CS_U01_EXPECT_FAILURE_CTEST_TIMEOUT} LABELS negative)
    endif()
endif()
