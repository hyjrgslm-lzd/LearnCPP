cmake_minimum_required(VERSION 3.25)
project(J1_eight_pitfalls LANGUAGES CXX)

# 如本目录被独立打开（不通过顶层 add_subdirectory），跳过
if(NOT TARGET stdexec::stdexec AND NOT COROUTINE_STUDY_STANDALONE)
    message(WARNING "J1_eight_pitfalls requires stage3 deps; configure via top-level CMakeLists.txt")
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict)
endif()

add_executable(J1_eight_pitfalls main.cpp)

# 本题不真正用 stdexec API，但为了与 stage3 其他题统一，仍链 stage3 deps
if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(J1_eight_pitfalls)
else()
    target_link_libraries(J1_eight_pitfalls PRIVATE stdexec::stdexec)
endif()

target_include_directories(J1_eight_pitfalls PRIVATE ${CMAKE_SOURCE_DIR}/include)
