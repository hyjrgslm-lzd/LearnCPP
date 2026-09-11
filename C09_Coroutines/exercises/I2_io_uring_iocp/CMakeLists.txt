cmake_minimum_required(VERSION 3.25)
project(I2_io_uring_iocp LANGUAGES CXX)

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
