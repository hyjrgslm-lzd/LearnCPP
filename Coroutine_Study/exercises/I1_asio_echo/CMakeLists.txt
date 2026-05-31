cmake_minimum_required(VERSION 3.25)
project(I1_asio_echo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8)
endif()

add_executable(I1_asio_echo main.cpp)
target_include_directories(I1_asio_echo PRIVATE ${CMAKE_SOURCE_DIR}/include)

# stage3 deps：Agent 1 的 helper 链入 standalone Asio
if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(I1_asio_echo)
endif()

# Asio standalone（header-only 时只需 include；如需 system 库链接由 helper 处理）
if(WIN32)
    target_link_libraries(I1_asio_echo PRIVATE ws2_32 mswsock)
endif()
