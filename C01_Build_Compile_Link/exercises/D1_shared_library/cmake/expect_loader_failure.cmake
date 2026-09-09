if(NOT DEFINED LOADER OR NOT DEFINED LIBRARY OR NOT DEFINED SYMBOL)
    message(FATAL_ERROR "LOADER, LIBRARY and SYMBOL are required")
endif()

execute_process(
    COMMAND "${LOADER}" "${LIBRARY}" "${SYMBOL}"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE run_err
)

set(_combined "${run_out}\n${run_err}")
if(run_result EQUAL 0)
    message(FATAL_ERROR "loader failure case unexpectedly succeeded")
endif()
if(NOT _combined MATCHES "missing export")
    message(FATAL_ERROR "loader failed for the wrong reason:\n${_combined}")
endif()

message(STATUS "loader rejected missing export as expected")
