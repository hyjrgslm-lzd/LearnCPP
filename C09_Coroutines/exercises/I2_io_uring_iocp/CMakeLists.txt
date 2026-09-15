cmake_minimum_required(VERSION 3.25)
project(I2_io_uring_iocp LANGUAGES CXX)

if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

if(WIN32)
    coroutine_study_require_option(COROUTINE_STUDY_ENABLE_IOCP "I2_io_uring_iocp")
else()
    coroutine_study_require_option(COROUTINE_STUDY_ENABLE_IO_URING "I2_io_uring_iocp")
endif()

if(WIN32)
    set(I2_SOURCES windows/main_iocp.cpp)
    set(I2_REFERENCE_SOURCES windows/solution_iocp.cpp)
    set(I2_LIBRARIES ws2_32 mswsock)
else()
    set(I2_SOURCES linux/main_io_uring.cpp)
    set(I2_REFERENCE_SOURCES linux/solution_io_uring.cpp)
    set(I2_LIBRARIES liburing::liburing)
endif()

coroutine_study_add_exercise(
    I2_io_uring_iocp
    SOURCES ${I2_SOURCES}
    REFERENCE_SOURCES ${I2_REFERENCE_SOURCES}
    LIBRARIES ${I2_LIBRARIES}
    STUDENT_TEST
)

learncpp_setup_ide("I2_io_uring_iocp")
