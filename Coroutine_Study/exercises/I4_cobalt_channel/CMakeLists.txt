cmake_minimum_required(VERSION 3.25)
project(I4_cobalt_channel LANGUAGES CXX)

# Boost.Cobalt 在 Windows 默认跳过：依赖完整 Boost + Asio
if(WIN32)
    message(STATUS "I4_cobalt_channel: skipped on Windows (Boost.Cobalt 配置复杂，默认 Linux/macOS)")
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(I4_cobalt_channel main.cpp)
target_include_directories(I4_cobalt_channel PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(I4_cobalt_channel)
else()
    find_package(Boost REQUIRED COMPONENTS cobalt)
    target_link_libraries(I4_cobalt_channel PRIVATE Boost::cobalt)
endif()
