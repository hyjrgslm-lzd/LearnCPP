if(NOT DEFINED CASE_SOURCE OR NOT DEFINED CASE_BUILD OR
        NOT DEFINED OBJECT_TARGET OR NOT DEFINED LINK_TARGET)
    message(FATAL_ERROR "CASE_SOURCE, CASE_BUILD, OBJECT_TARGET, and LINK_TARGET are required")
endif()

if(NOT DEFINED GENERATOR OR GENERATOR STREQUAL "")
    set(_generator_args)
else()
    set(_generator_args -G "${GENERATOR}")
endif()

if(DEFINED PLATFORM_ARG AND NOT PLATFORM_ARG STREQUAL "")
    list(APPEND _generator_args -A "${PLATFORM_ARG}")
endif()

if(DEFINED MAKE_PROGRAM AND NOT MAKE_PROGRAM STREQUAL "")
    set(_make_program_arg -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM})
else()
    set(_make_program_arg)
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${CASE_SOURCE}" -B "${CASE_BUILD}" ${_generator_args} ${_make_program_arg}
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_out
    ERROR_VARIABLE configure_err
)

if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "negative case configure failed before object compilation:\n${configure_out}\n${configure_err}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${CASE_BUILD}" --target "${OBJECT_TARGET}" --config Debug
    RESULT_VARIABLE object_result
    OUTPUT_VARIABLE object_out
    ERROR_VARIABLE object_err
)

if(NOT object_result EQUAL 0)
    message(FATAL_ERROR "negative case object compilation failed before the expected link step:\n${object_out}\n${object_err}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${CASE_BUILD}" --target "${LINK_TARGET}" --config Debug
    RESULT_VARIABLE link_result
    OUTPUT_VARIABLE link_out
    ERROR_VARIABLE link_err
)

set(_combined "${link_out}\n${link_err}")

if(link_result EQUAL 0)
    message(FATAL_ERROR "negative case unexpectedly linked successfully")
endif()

set(_msvc_duplicate FALSE)
if(_combined MATCHES "lesson_value" AND
        _combined MATCHES "(LNK2005|LNK1169)")
    set(_msvc_duplicate TRUE)
endif()

set(_gnu_duplicate FALSE)
if(_combined MATCHES "lesson_value" AND
        _combined MATCHES "multiple definition")
    set(_gnu_duplicate TRUE)
endif()

if(NOT _msvc_duplicate AND NOT _gnu_duplicate)
    message(FATAL_ERROR "negative case failed, but not as a duplicate-symbol link failure for lesson_value:\n${_combined}")
endif()

message(STATUS "negative case compiled objects, then failed at the expected duplicate-symbol link step")
message(STATUS "${_combined}")


