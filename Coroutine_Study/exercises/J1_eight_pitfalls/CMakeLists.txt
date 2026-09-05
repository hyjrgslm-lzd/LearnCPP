cmake_minimum_required(VERSION 3.28)
project(J1_eight_pitfalls LANGUAGES CXX)

if(NOT COMMAND coroutine_study_add_exercise)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
    include(AddExercise)
endif()

coroutine_study_add_exercise(
    J1_eight_pitfalls
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
)

if(BUILD_TESTING AND NOT COROUTINE_STUDY_ENABLE_UNSAFE_DEMOS)
    add_test(NAME J1_eight_pitfalls COMMAND J1_eight_pitfalls)
endif()

foreach(target J1_eight_pitfalls J1_eight_pitfalls_reference)
    if(TARGET ${target} AND MSVC)
        target_compile_options(${target} PRIVATE /Zc:__cplusplus /utf-8 /await:strict)
    endif()
    if(TARGET ${target} AND COROUTINE_STUDY_ENABLE_UNSAFE_DEMOS)
        target_compile_definitions(${target} PRIVATE COROUTINE_STUDY_ENABLE_UNSAFE_DEMOS=1)
    endif()
endforeach()
