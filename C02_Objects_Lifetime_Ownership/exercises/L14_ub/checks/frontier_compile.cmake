cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED BUILD_DIR OR NOT DEFINED TARGET OR NOT DEFINED CASE OR NOT DEFINED EXPECT OR NOT DEFINED OUT_DIR)
    message(FATAL_ERROR "missing frontier probe argument")
endif()
if(NOT DEFINED CONFIG)
    set(CONFIG "")
endif()
if(NOT DEFINED SOURCE_MARKER)
    set(SOURCE_MARKER "")
endif()
if(NOT DEFINED EXPECTED_FAILURE)
    set(EXPECTED_FAILURE "")
endif()
if(NOT DEFINED ACCEPTED_DIAGNOSTIC)
    set(ACCEPTED_DIAGNOSTIC "")
endif()
if(NOT DEFINED COMMAND_TIMEOUT_SECONDS)
    set(COMMAND_TIMEOUT_SECONDS 60)
endif()

file(MAKE_DIRECTORY "${OUT_DIR}")
string(TIMESTAMP stamp "%Y%m%d-%H%M%S" UTC)
set(base_out "${OUT_DIR}/${CASE}-${stamp}.txt")
set(out_file "${base_out}")
set(counter 0)
while(EXISTS "${out_file}")
    math(EXPR counter "${counter} + 1")
    set(out_file "${OUT_DIR}/${CASE}-${stamp}-${counter}.txt")
endwhile()
set(index_file "${OUT_DIR}/index.txt")

set(command "${CMAKE_COMMAND}" --build "${BUILD_DIR}")
if(NOT "${CONFIG}" STREQUAL "")
    list(APPEND command --config "${CONFIG}")
endif()
list(APPEND command --target "${TARGET}")

execute_process(
    COMMAND ${command}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    TIMEOUT "${COMMAND_TIMEOUT_SECONDS}"
)

string(CONCAT combined "${stdout}\n${stderr}")
# Match diagnostic messages, not a target/source name containing "assignment".
string(REGEX MATCHALL "([Ww]arning|[Ee]rror|fatal error)( [A-Z]+[0-9]+)?:[^\r\n]*" diagnostic_lines "${combined}")
string(JOIN "\n" diagnostics ${diagnostic_lines})
file(WRITE "${out_file}"
"case=${CASE}\n"
"target=${TARGET}\n"
"expect=${EXPECT}\n"
"source_marker=${SOURCE_MARKER}\n"
"expected_failure=${EXPECTED_FAILURE}\n"
"accepted_diagnostic=${ACCEPTED_DIAGNOSTIC}\n"
"result=${result}\n"
"command=${command}\n"
"stdout<<EOF\n${stdout}\nEOF\n"
"stderr<<EOF\n${stderr}\nEOF\n")
file(APPEND "${index_file}" "${stamp} ${CASE} ${TARGET} result=${result} file=${out_file}\n")

if(NOT "${result}" MATCHES "^[0-9]+$")
    message(FATAL_ERROR "${CASE}: build did not return a process exit code (${result}); see ${out_file}")
endif()

function(require_source_marker)
    if(NOT "${SOURCE_MARKER}" STREQUAL "" AND NOT combined MATCHES "${SOURCE_MARKER}")
        message(FATAL_ERROR "${CASE}: failure did not mention expected source '${SOURCE_MARKER}'; see ${out_file}")
    endif()
endfunction()

function(require_expected_failure)
    require_source_marker()
    if("${EXPECTED_FAILURE}" STREQUAL "")
        message(FATAL_ERROR "${CASE}: expected failure pattern is empty; see ${out_file}")
    endif()
    if(NOT diagnostics MATCHES "${EXPECTED_FAILURE}")
        message(FATAL_ERROR "${CASE}: failure did not match expected diagnostic '${EXPECTED_FAILURE}'; see ${out_file}")
    endif()
endfunction()

if(EXPECT STREQUAL "must_pass")
    if(NOT result EQUAL 0)
        require_source_marker()
        message(FATAL_ERROR "${CASE}: baseline/toolchain compile failed; see ${out_file}")
    endif()
    message(STATUS "PASS: ${CASE} baseline compiled; see ${out_file}")
elseif(EXPECT STREQUAL "capability_compile")
    if(result EQUAL 0)
        message(STATUS "PASS: ${CASE} capability compiled; see ${out_file}")
    else()
        require_expected_failure()
        message(STATUS "SKIP: ${CASE} capability unavailable with expected diagnostic; see ${out_file}")
    endif()
elseif(EXPECT STREQUAL "capability_reject")
    if(result EQUAL 0)
        if(NOT "${ACCEPTED_DIAGNOSTIC}" STREQUAL "" AND diagnostics MATCHES "${ACCEPTED_DIAGNOSTIC}")
            message(STATUS "SKIP: ${CASE} accepted after expected diagnostic; see ${out_file}")
        else()
            message(STATUS "SKIP: ${CASE} accepted without expected rejection diagnostic; see ${out_file}")
        endif()
    else()
        require_expected_failure()
        message(STATUS "PASS: ${CASE} rejected with expected diagnostic; see ${out_file}")
    endif()
elseif(EXPECT STREQUAL "observe_compile")
    if(NOT result EQUAL 0)
        require_source_marker()
        message(FATAL_ERROR "${CASE}: compile-only review model failed; see ${out_file}")
    endif()
    message(STATUS "PASS: ${CASE} compile-only review model built; no runtime support claim; see ${out_file}")
else()
    message(FATAL_ERROR "unknown EXPECT=${EXPECT}; see ${out_file}")
endif()
