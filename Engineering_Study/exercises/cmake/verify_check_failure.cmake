if(NOT DEFINED PROGRAM)
    message(FATAL_ERROR "PROGRAM is required")
endif()
execute_process(COMMAND "${PROGRAM}" --fail
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    TIMEOUT 5)
if(NOT "${result}" STREQUAL "1")
    message(FATAL_ERROR "check must fail normally with exit 1, not a crash/timeout: ${result}\n${output}\n${error}")
endif()
if(NOT error MATCHES "check failed: intentional check self-test failure")
    message(FATAL_ERROR "missing meaningful failure diagnostic: ${error}")
endif()
