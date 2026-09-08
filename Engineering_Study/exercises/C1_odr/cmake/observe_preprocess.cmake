if(NOT DEFINED SOURCE_DIR OR NOT DEFINED BINARY_DIR OR NOT DEFINED CXX_COMPILER)
    message(FATAL_ERROR "SOURCE_DIR, BINARY_DIR, and CXX_COMPILER are required")
endif()

set(_source "${SOURCE_DIR}/observation/two_tu_symbols/observer_a.cpp")
set(_include "${SOURCE_DIR}/observation/two_tu_symbols")

if(COMPILER_ID STREQUAL "MSVC")
    set(_command "${CXX_COMPILER}" /nologo /EP /TP /I "${_include}" "${_source}")
else()
    set(_command "${CXX_COMPILER}" -E -P -I "${_include}" "${_source}")
endif()

execute_process(
    COMMAND ${_command}
    RESULT_VARIABLE preprocess_result
    OUTPUT_VARIABLE preprocess_out
    ERROR_VARIABLE preprocess_err
)

if(NOT preprocess_result EQUAL 0)
    message(FATAL_ERROR "preprocess failed:\n${preprocess_out}\n${preprocess_err}")
endif()

if(NOT preprocess_out MATCHES "int[ \t\r\n]+lesson_value[ \t\r\n]*\\(")
    message(FATAL_ERROR "preprocessed output did not contain lesson_value definition")
endif()

file(MAKE_DIRECTORY "${BINARY_DIR}/evidence")
file(WRITE "${BINARY_DIR}/evidence/preprocess_observer_a.txt" "${preprocess_out}")
message(STATUS "preprocess evidence contains lesson_value definition")

