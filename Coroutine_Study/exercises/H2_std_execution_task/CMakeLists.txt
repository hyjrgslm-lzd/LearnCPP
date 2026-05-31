cmake_minimum_required(VERSION 3.25)
project(H2_std_execution_task LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8)
endif()

add_executable(H2_std_execution_task main.cpp)
target_include_directories(H2_std_execution_task PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(H2_std_execution_task)
else()
    target_link_libraries(H2_std_execution_task PRIVATE stdexec::stdexec)
endif()
