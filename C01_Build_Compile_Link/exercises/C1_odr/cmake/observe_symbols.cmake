if(NOT DEFINED BINARY_DIR)
    message(FATAL_ERROR "BINARY_DIR is required")
endif()
if(NOT DEFINED BUILD_ROOT)
    set(BUILD_ROOT "${BINARY_DIR}")
endif()

if(NOT DEFINED CONFIG OR CONFIG STREQUAL "")
    set(CONFIG Debug)
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${BUILD_ROOT}" --target C1_odr_observation_objects --config "${CONFIG}"
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_out
    ERROR_VARIABLE build_err
)

if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "object build failed:\n${build_out}\n${build_err}")
endif()

file(GLOB_RECURSE _observer_a_objects
    "${BINARY_DIR}/*observer_a*.obj"
    "${BINARY_DIR}/*observer_a*.o"
)
file(GLOB_RECURSE _observer_b_objects
    "${BINARY_DIR}/*observer_b*.obj"
    "${BINARY_DIR}/*observer_b*.o"
)

function(_select_config_object out_var candidate_objects object_name)
    list(LENGTH candidate_objects candidate_count)
    if(candidate_count EQUAL 1)
        set(${out_var} "${candidate_objects}" PARENT_SCOPE)
        return()
    endif()

    set(config_matches)
    foreach(candidate IN LISTS candidate_objects)
        file(TO_CMAKE_PATH "${candidate}" candidate_path)
        if(candidate_path MATCHES "/${CONFIG}/")
            list(APPEND config_matches "${candidate}")
        endif()
    endforeach()

    list(LENGTH config_matches config_count)
    if(config_count EQUAL 1)
        set(${out_var} "${config_matches}" PARENT_SCOPE)
        return()
    endif()

    message(FATAL_ERROR "expected exactly one ${object_name} object for config ${CONFIG}, found ${config_count} matching config out of ${candidate_count}: ${candidate_objects}")
endfunction()

_select_config_object(_observer_a_object "${_observer_a_objects}" observer_a)
_select_config_object(_observer_b_object "${_observer_b_objects}" observer_b)

function(_check_lesson_value_definition object)
    if(DEFINED DUMPBIN AND EXISTS "${DUMPBIN}")
        execute_process(
            COMMAND "${DUMPBIN}" /symbols "${object}"
            RESULT_VARIABLE symbol_result
            OUTPUT_VARIABLE symbol_out
            ERROR_VARIABLE symbol_err
        )
        set(symbol_kind "dumpbin")
        set(definition_regex "SECT[0-9A-Fa-f]+[^\n\r]*External[^\n\r]*lesson_value")
    elseif(DEFINED NM AND EXISTS "${NM}")
        execute_process(
            COMMAND "${NM}" -C "${object}"
            RESULT_VARIABLE symbol_result
            OUTPUT_VARIABLE symbol_out
            ERROR_VARIABLE symbol_err
        )
        set(symbol_kind "nm")
        set(definition_regex "(^|\n)[0-9A-Fa-f]+[ \t]+[TtWw][ \t]+.*lesson_value")
    else()
        message(FATAL_ERROR "neither dumpbin nor nm is available for symbol observation")
    endif()

    if(NOT symbol_result EQUAL 0)
        message(FATAL_ERROR "symbol observation failed for ${object}:\n${symbol_out}\n${symbol_err}")
    endif()

    if(NOT symbol_out MATCHES "${definition_regex}")
        message(FATAL_ERROR "${symbol_kind} output for ${object} did not contain an external lesson_value definition:\n${symbol_out}\n${symbol_err}")
    endif()

    set(_symbol_report "${_symbol_report}\n==== ${object} ====\n${symbol_out}\n${symbol_err}" PARENT_SCOPE)
endfunction()

set(_symbol_report "")
_check_lesson_value_definition("${_observer_a_object}")
_check_lesson_value_definition("${_observer_b_object}")

file(MAKE_DIRECTORY "${BINARY_DIR}/evidence")
file(WRITE "${BINARY_DIR}/evidence/symbols.txt" "${_symbol_report}")
message(STATUS "symbol evidence contains external lesson_value definitions in observer_a and observer_b objects")

