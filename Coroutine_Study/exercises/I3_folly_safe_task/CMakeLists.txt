cmake_minimum_required(VERSION 3.25)
project(I3_folly_safe_task LANGUAGES CXX)

# Folly 在 Windows 默认跳过：依赖 vcpkg + 大量 POSIX-isms
if(WIN32)
    message(STATUS "I3_folly_safe_task: skipped on Windows (Folly 仅 Linux/macOS)")
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(I3_folly_safe_task main.cpp)
target_include_directories(I3_folly_safe_task PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(I3_folly_safe_task)
endif()
