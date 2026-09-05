cmake_minimum_required(VERSION 3.25)
project(H1_as_awaitable LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /Zc:preprocessor)
endif()

if(COMMAND coroutine_study_add_exercise)
    coroutine_study_add_exercise(
        H1_as_awaitable
        SOURCES main.cpp
        REFERENCE_SOURCES solution.cpp
        LIBRARIES stdexec::stdexec
    )
else()
    add_executable(H1_as_awaitable main.cpp)
    target_link_libraries(H1_as_awaitable PRIVATE stdexec::stdexec)
    add_executable(H1_as_awaitable_reference solution.cpp)
    target_link_libraries(H1_as_awaitable_reference PRIVATE stdexec::stdexec)
endif()
