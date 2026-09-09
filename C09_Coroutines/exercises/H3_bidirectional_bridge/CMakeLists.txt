cmake_minimum_required(VERSION 3.25)
project(H3_bidirectional_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /Zc:preprocessor)
endif()

if(COMMAND coroutine_study_add_exercise)
    coroutine_study_add_exercise(
        H3_bidirectional_bridge
        SOURCES main.cpp
        REFERENCE_SOURCES solution.cpp
        LIBRARIES stdexec::stdexec
    )
else()
    add_executable(H3_bidirectional_bridge main.cpp)
    target_link_libraries(H3_bidirectional_bridge PRIVATE stdexec::stdexec)
    add_executable(H3_bidirectional_bridge_reference solution.cpp)
    target_link_libraries(H3_bidirectional_bridge_reference PRIVATE stdexec::stdexec)
endif()
