cmake_minimum_required(VERSION 3.28)
project(F03_native_facilities LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

if(CONCURRENCY_STUDY_BUILD_REFERENCE)
    function(f03_add_native_subject subject)
        add_executable(F03_${subject}_reference ${subject}.cpp)
        cs_configure_target(F03_${subject}_reference)
        target_compile_definitions(F03_${subject}_reference PRIVATE
            CS_HAS_STD_THREAD_ATTRIBUTES=${CS_HAS_STD_THREAD_ATTRIBUTES}
            CS_HAS_STD_HAZARD_POINTER_BATCH=${CS_HAS_STD_HAZARD_POINTER_BATCH})
        if(BUILD_TESTING)
            add_test(NAME F03_${subject}_reference COMMAND F03_${subject}_reference)
            set_tests_properties(F03_${subject}_reference PROPERTIES
                TIMEOUT ${CS_TEST_TIMEOUT}
                SKIP_RETURN_CODE 77
                LABELS "reference;native")
        endif()
    endfunction()

    f03_add_native_subject(std_senders)
    f03_add_native_subject(std_hazard_pointer)
    f03_add_native_subject(std_rcu)
endif()
