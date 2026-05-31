cmake_minimum_required(VERSION 3.25)
project(I2_io_uring_iocp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8)
endif()

# 双版本：Linux 走 io_uring；Windows 走 IOCP。两份 main.cpp 各居一隅。
if(WIN32)
    add_executable(I2_io_uring_iocp windows/main_iocp.cpp)
    target_link_libraries(I2_io_uring_iocp PRIVATE ws2_32)
else()
    add_executable(I2_io_uring_iocp linux/main_io_uring.cpp)
    # liburing 必须由系统包提供：sudo apt install liburing-dev
    target_link_libraries(I2_io_uring_iocp PRIVATE uring)
endif()

target_include_directories(I2_io_uring_iocp PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(COMMAND coroutine_study_link_stage3_deps)
    coroutine_study_link_stage3_deps(I2_io_uring_iocp)
endif()
