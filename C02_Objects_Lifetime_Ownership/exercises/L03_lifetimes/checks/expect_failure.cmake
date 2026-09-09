foreach(required EXECUTABLE EXPECTED_TEXT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

execute_process(
    COMMAND "${EXECUTABLE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    TIMEOUT 30
)

if(result EQUAL 0)
    message(FATAL_ERROR "expected failure, got exit 0")
endif()
if(result MATCHES "timeout")
    message(FATAL_ERROR "expected diagnostic failure, got timeout")
endif()

string(CONCAT output "${stdout}" "${stderr}")
string(FIND "${output}" "${EXPECTED_TEXT}" found_at)
if(found_at EQUAL -1)
    message(FATAL_ERROR "expected diagnostic not found: ${EXPECTED_TEXT}")
endif()
