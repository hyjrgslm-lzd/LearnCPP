cmake_minimum_required(VERSION 3.28)
project(L07_readiness LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
set(c07_l07_libs)
if(WIN32)
    list(APPEND c07_l07_libs ws2_32)
endif()
c07_add_exercise(NAME L07_readiness HEADER readiness.hpp CHECK checks/readiness_checks.cpp
    BAD_DIAGNOSTIC "runtime payload matches drained bytes"
    LIBRARIES ${c07_l07_libs})

if(C07_STUDY_BUILD_REFERENCE)
    add_executable(L07_readiness_validation_scripted_bad checks/readiness_checks.cpp)
    c07_configure_target(L07_readiness_validation_scripted_bad)
    target_include_directories(L07_readiness_validation_scripted_bad PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/validation/scripted_bad"
        "${CMAKE_CURRENT_SOURCE_DIR}/checks")
    if(c07_l07_libs)
        target_link_libraries(L07_readiness_validation_scripted_bad PRIVATE ${c07_l07_libs})
    endif()
    c07_add_test(L07_readiness_validation_scripted_bad_rejected
        COMMAND "$<TARGET_FILE:L07_readiness_validation_scripted_bad>"
        EXPECT_EXIT 1
        CONTAINS "runtime payload matches drained bytes"
        LABELS negative)
endif()
