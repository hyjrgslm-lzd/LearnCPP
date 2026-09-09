foreach(required CASE_SOURCE CASE_BUILD EXPECTED_TEXT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

execute_process(
    COMMAND ${CMAKE_COMMAND} -S "${CASE_SOURCE}" -B "${CASE_BUILD}" -G "${GENERATOR}" -A "${PLATFORM}"
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_stdout
    ERROR_VARIABLE configure_stderr
    TIMEOUT 60
)

if(configure_result EQUAL 0)
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build "${CASE_BUILD}" --config "${CONFIG}" --clean-first
        RESULT_VARIABLE build_result
        OUTPUT_VARIABLE build_stdout
        ERROR_VARIABLE build_stderr
        TIMEOUT 120
    )
else()
    set(build_result ${configure_result})
endif()

string(CONCAT output "${configure_stdout}" "${configure_stderr}" "${build_stdout}" "${build_stderr}")
if(build_result EQUAL 0)
    message(FATAL_ERROR "expected build failure, got success")
endif()

string(FIND "${output}" "${EXPECTED_TEXT}" found_at)
if(found_at EQUAL -1)
    message(FATAL_ERROR "expected diagnostic not found: ${EXPECTED_TEXT}")
endif()
