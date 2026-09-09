if(NOT DEFINED BUILD_DIR)
    message(FATAL_ERROR "BUILD_DIR is required")
endif()
if(NOT DEFINED TARGET)
    message(FATAL_ERROR "TARGET is required")
endif()
execute_process(
    COMMAND ${CMAKE_COMMAND} --build "${BUILD_DIR}" --config "${CONFIG}" --target "${TARGET}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    TIMEOUT 60
)
if(result EQUAL 0)
    message(FATAL_ERROR "throwing move-only target unexpectedly built")
endif()
string(CONCAT output "${stdout}" "${stderr}")
string(FIND "${output}" "copy constructible or nothrow move constructible" found_at)
if(found_at EQUAL -1)
    message(FATAL_ERROR "expected static_assert diagnostic not found")
endif()
