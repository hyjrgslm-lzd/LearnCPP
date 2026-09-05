coroutine_study_add_exercise(G2_when_all_impl REFERENCE_SOURCES solution.cpp)

if(BUILD_TESTING AND COROUTINE_STUDY_BUILD_REFERENCE)
    set_tests_properties(G2_when_all_impl_reference PROPERTIES TIMEOUT 5)
endif()
