cmake_minimum_required(VERSION 3.28)
project(J2_cross_compiler_abi LANGUAGES CXX)

if(NOT COMMAND coroutine_study_add_exercise)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
    include(AddExercise)
endif()

coroutine_study_add_exercise(
    J2_cross_compiler_abi
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
)

foreach(target J2_cross_compiler_abi J2_cross_compiler_abi_reference)
    if(TARGET ${target} AND MSVC)
        target_compile_options(${target} PRIVATE /Zc:__cplusplus /utf-8 /await:strict /Zi)
    elseif(TARGET ${target} AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE -Rpass=coroutine-elide -Rpass-missed=coroutine-elide)
    endif()
endforeach()
