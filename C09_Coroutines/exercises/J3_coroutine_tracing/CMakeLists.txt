cmake_minimum_required(VERSION 3.28)
project(J3_coroutine_tracing LANGUAGES CXX)

if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

if(NOT COMMAND coroutine_study_add_exercise)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
    include(AddExercise)
endif()

coroutine_study_add_exercise(
    J3_coroutine_tracing
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
)

foreach(target J3_coroutine_tracing J3_coroutine_tracing_reference)
    if(TARGET ${target} AND MSVC)
        target_compile_options(${target} PRIVATE /Zc:__cplusplus /utf-8 /await:strict /Zi)
    endif()
endforeach()

learncpp_setup_ide("J3_coroutine_tracing")
