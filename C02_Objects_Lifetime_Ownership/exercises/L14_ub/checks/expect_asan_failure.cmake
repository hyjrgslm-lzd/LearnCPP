cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED EXECUTABLE OR NOT DEFINED EXPECTED_TEXT OR NOT DEFINED EXPECTED_SOURCE)
    message(FATAL_ERROR "missing ASan failure check argument")
endif()
if(NOT DEFINED TIMEOUT_SECONDS)
    set(TIMEOUT_SECONDS 10)
endif()

execute_process(
    COMMAND "${EXECUTABLE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    TIMEOUT "${TIMEOUT_SECONDS}"
)

string(CONCAT combined "${stdout}\n${stderr}")
if(NOT "${result}" MATCHES "^[0-9]+$")
    message(FATAL_ERROR "ASan diagnostic process did not return a process exit code (${result}); output was:\n${combined}")
endif()
if(result EQUAL 0)
    message(FATAL_ERROR "expected ASan diagnostic process to fail, but it exited 0")
endif()
if(NOT combined MATCHES "${EXPECTED_TEXT}")
    message(FATAL_ERROR "expected ASan diagnostic text '${EXPECTED_TEXT}' not found; output was:\n${combined}")
endif()
if(NOT combined MATCHES "${EXPECTED_SOURCE}")
    message(FATAL_ERROR "expected ASan source marker '${EXPECTED_SOURCE}' not found; output was:\n${combined}")
endif()
message(STATUS "PASS: ASan diagnostic matched ${EXPECTED_TEXT} and ${EXPECTED_SOURCE}")
