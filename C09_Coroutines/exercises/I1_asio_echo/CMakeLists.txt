cmake_minimum_required(VERSION 3.25)
project(I1_asio_echo LANGUAGES CXX)

if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

coroutine_study_require_option(COROUTINE_STUDY_ENABLE_ASIO "I1_asio_echo")

set(I1_LIBRARIES asio::asio)
if(WIN32)
    list(APPEND I1_LIBRARIES ws2_32 mswsock)
endif()

coroutine_study_add_exercise(
    I1_asio_echo
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    LIBRARIES ${I1_LIBRARIES}
    STUDENT_TEST
)

learncpp_setup_ide("I1_asio_echo")
