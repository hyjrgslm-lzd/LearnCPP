cmake_minimum_required(VERSION 3.25)
project(Capstone4_rpc_framework LANGUAGES CXX)

include(CTest)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# 如本目录被独立打开（不通过顶层 add_subdirectory），跳过
if(NOT TARGET asio::asio AND NOT COROUTINE_STUDY_STANDALONE)
    message(WARNING "Capstone4_rpc_framework requires asio; configure via top-level CMakeLists.txt")
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict /Zi /bigobj)
endif()

set(CAPSTONE4_RPC_STUDENT_SOURCES
    src/protocol.cpp
    src/server.cpp
    src/client.cpp
)

add_library(Capstone4_rpc_framework_student_lib STATIC ${CAPSTONE4_RPC_STUDENT_SOURCES})
target_link_libraries(Capstone4_rpc_framework_student_lib PUBLIC asio::asio)

target_include_directories(Capstone4_rpc_framework_student_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(Capstone4_rpc_framework src/main.cpp)
target_link_libraries(Capstone4_rpc_framework PRIVATE Capstone4_rpc_framework_student_lib)

add_executable(Capstone4_rpc_framework_student_check
    tests/rpc_student_test.cpp
)
target_link_libraries(Capstone4_rpc_framework_student_check PRIVATE Capstone4_rpc_framework_student_lib)

if(BUILD_TESTING AND COROUTINE_STUDY_TEST_STARTERS)
    add_test(NAME Capstone4_rpc_framework_student_check COMMAND Capstone4_rpc_framework_student_check)
    set_tests_properties(Capstone4_rpc_framework_student_check PROPERTIES
        LABELS "starter;rpc"
        TIMEOUT 60
    )
endif()

if(BUILD_TESTING AND COROUTINE_STUDY_BUILD_REFERENCE)
    add_executable(Capstone4_rpc_framework_answer_check
        tests/rpc_student_test.cpp
        good/protocol.cpp
        good/client.cpp
        good/server.cpp
    )
    target_include_directories(Capstone4_rpc_framework_answer_check PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(Capstone4_rpc_framework_answer_check PRIVATE asio::asio)
    add_test(NAME Capstone4_rpc_framework_answer_check COMMAND Capstone4_rpc_framework_answer_check)
    set_tests_properties(Capstone4_rpc_framework_answer_check PROPERTIES
        LABELS "answer;reference_adapter;rpc"
        TIMEOUT 60
    )

    add_executable(Capstone4_rpc_framework_protocol_good_check
        tests/rpc_student_test.cpp
        good/client.cpp
        good/server.cpp
        good/protocol_alt.cpp
    )
    target_include_directories(Capstone4_rpc_framework_protocol_good_check PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(Capstone4_rpc_framework_protocol_good_check PRIVATE asio::asio)
    add_test(NAME Capstone4_rpc_framework_protocol_good_check COMMAND Capstone4_rpc_framework_protocol_good_check)
    set_tests_properties(Capstone4_rpc_framework_protocol_good_check PROPERTIES
        LABELS "good;protocol;rpc"
        TIMEOUT 60
    )

    add_executable(Capstone4_rpc_framework_bad_newline_check
        tests/rpc_student_test.cpp
        good/client.cpp
        good/server.cpp
        bad/protocol_newline.cpp
    )
    target_include_directories(Capstone4_rpc_framework_bad_newline_check PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(Capstone4_rpc_framework_bad_newline_check PRIVATE asio::asio)
    add_test(NAME Capstone4_rpc_framework_bad_newline_rejected
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/../../C07_OS_Memory_System_IO/exercises/tools/run_test.py
            --name Capstone4_rpc_framework_bad_newline_rejected
            --records ${CMAKE_CURRENT_BINARY_DIR}/validation
            --timeout 50
            --expect-exit 1
            --contains "check failed: frame header uses 8-byte decimal length"
            -- $<TARGET_FILE:Capstone4_rpc_framework_bad_newline_check>
    )
    set_tests_properties(Capstone4_rpc_framework_bad_newline_rejected PROPERTIES
        LABELS "bad;rpc"
        TIMEOUT 60
    )
endif()

if(COROUTINE_STUDY_BUILD_REFERENCE)
    add_executable(Capstone4_rpc_framework_reference
        reference/tests/rpc_reference_test.cpp
    )
    target_include_directories(Capstone4_rpc_framework_reference PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/reference/include
    )
    target_link_libraries(Capstone4_rpc_framework_reference PRIVATE asio::asio)
    add_test(NAME Capstone4_rpc_framework_reference COMMAND Capstone4_rpc_framework_reference)
    set_tests_properties(Capstone4_rpc_framework_reference PROPERTIES
        LABELS "reference;rpc"
        TIMEOUT 60
    )
endif()
