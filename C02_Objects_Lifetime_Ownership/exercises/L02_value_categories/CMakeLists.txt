cmake_minimum_required(VERSION 3.28)
project(L02_value_categories LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

add_executable(L02_value_categories_observation checks/observation.cpp)
core_configure_target(L02_value_categories_observation)
core_add_test(L02_value_categories_observation COMMAND L02_value_categories_observation LABELS observation TIMEOUT 30)

function(l02_negative name expected)
    set(generator_args -DGENERATOR=${CMAKE_GENERATOR})
    if(CMAKE_GENERATOR_PLATFORM)
        list(APPEND generator_args -DPLATFORM=${CMAKE_GENERATOR_PLATFORM})
    endif()
    add_test(NAME L02_value_categories_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCASE_SOURCE=${CMAKE_CURRENT_LIST_DIR}/negative/${name}
            -DCASE_BUILD=${CMAKE_CURRENT_BINARY_DIR}/negative/${name}
            -DEXPECTED_TEXT=${expected}
            ${generator_args}
            -DCONFIG=$<CONFIG>
            -P ${CMAKE_CURRENT_LIST_DIR}/checks/expect_build_failure.cmake
    )
    set_tests_properties(L02_value_categories_negative_${name} PROPERTIES LABELS negative TIMEOUT 180)
endfunction()

l02_negative(bind_rvalue_to_nonconst_lvalue "C2440")
l02_negative(forward_lvalue_to_rvalue "C2664")
