cmake_minimum_required(VERSION 3.28)
project(G2_when_all_impl LANGUAGES CXX)
if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_add_exercise(G2_when_all_impl STUDENT_TEST REFERENCE_SOURCES solution.cpp)

if(BUILD_TESTING AND COROUTINE_STUDY_BUILD_REFERENCE)
    set_tests_properties(G2_when_all_impl_reference PROPERTIES TIMEOUT 5)
endif()

if(BUILD_TESTING AND COROUTINE_STUDY_TEST_STARTERS)
    coroutine_study_set_test_defaults(G2_when_all_impl starter 5)
endif()

learncpp_setup_ide("G2_when_all_impl")
