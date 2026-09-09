cmake_minimum_required(VERSION 3.28)
project(L06_move_return LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

add_executable(L06_move_return_observation checks/observation.cpp)
core_configure_target(L06_move_return_observation)
core_add_test(L06_move_return_observation COMMAND L06_move_return_observation LABELS observation TIMEOUT 30)

function(l06_negative name expected)
    set(generator_args -DGENERATOR=${CMAKE_GENERATOR})
    if(CMAKE_GENERATOR_PLATFORM)
        list(APPEND generator_args -DPLATFORM=${CMAKE_GENERATOR_PLATFORM})
    endif()
    add_test(NAME L06_move_return_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCASE_SOURCE=${CMAKE_CURRENT_LIST_DIR}/negative/${name}
            -DCASE_BUILD=${CMAKE_CURRENT_BINARY_DIR}/negative/${name}
            -DEXPECTED_TEXT=${expected}
            ${generator_args}
            -DCONFIG=$<CONFIG>
            -P ${CMAKE_CURRENT_LIST_DIR}/checks/expect_build_failure.cmake
    )
    set_tests_properties(L06_move_return_negative_${name} PROPERTIES LABELS negative TIMEOUT 180)
endfunction()

l06_negative(return_named_deleted "deleted")
l06_negative(const_move_only "C2280")
