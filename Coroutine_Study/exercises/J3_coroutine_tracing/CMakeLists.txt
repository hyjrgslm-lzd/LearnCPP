cmake_minimum_required(VERSION 3.25)
project(J3_coroutine_tracing LANGUAGES CXX)

# 如本目录被独立打开（不通过顶层 add_subdirectory），跳过
if(NOT TARGET stdexec::stdexec AND NOT COROUTINE_STUDY_STANDALONE)
    message(WARNING "J3_coroutine_tracing requires stage3 deps; configure via top-level CMakeLists.txt")
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict /Zi)
endif()

add_executable(J3_coroutine_tracing main.cpp)

if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(J3_coroutine_tracing)
else()
    target_link_libraries(J3_coroutine_tracing PRIVATE stdexec::stdexec)
endif()

target_include_directories(J3_coroutine_tracing PRIVATE ${CMAKE_SOURCE_DIR}/include)
