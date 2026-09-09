cmake_minimum_required(VERSION 3.28)
project(L01_initialization LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

add_executable(L01_initialization_observation checks/observation.cpp)
core_configure_target(L01_initialization_observation)
core_add_test(L01_initialization_observation COMMAND L01_initialization_observation LABELS observation TIMEOUT 30)

function(l01_negative name expected)
    set(generator_args -DGENERATOR=${CMAKE_GENERATOR})
    if(CMAKE_GENERATOR_PLATFORM)
        list(APPEND generator_args -DPLATFORM=${CMAKE_GENERATOR_PLATFORM})
    endif()
    add_test(NAME L01_initialization_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCASE_SOURCE=${CMAKE_CURRENT_LIST_DIR}/negative/${name}
            -DCASE_BUILD=${CMAKE_CURRENT_BINARY_DIR}/negative/${name}
            -DEXPECTED_TEXT=${expected}
            ${generator_args}
            -DCONFIG=$<CONFIG>
            -P ${CMAKE_CURRENT_LIST_DIR}/checks/expect_build_failure.cmake
    )
    set_tests_properties(L01_initialization_negative_${name} PROPERTIES LABELS negative TIMEOUT 180)
endfunction()

l01_negative(narrowing "narrowing")
l01_negative(constinit_dynamic "constinit")
