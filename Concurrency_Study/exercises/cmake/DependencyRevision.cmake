include_guard(GLOBAL)
function(cs_verify_checkout directory expected label)
    find_package(Git REQUIRED)
    get_filename_component(_cs_checkout "${directory}" REALPATH)
    if(NOT EXISTS "${_cs_checkout}/.git")
        message(FATAL_ERROR "${label}: supply a Git checkout at ${expected}, or enable fetching the pinned source")
    endif()
    execute_process(COMMAND "${GIT_EXECUTABLE}" -c "safe.directory=${_cs_checkout}"
        -C "${_cs_checkout}" rev-parse HEAD OUTPUT_VARIABLE revision
        OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE result ERROR_QUIET)
    if(NOT result EQUAL 0 OR NOT revision STREQUAL expected)
        message(FATAL_ERROR "${label}: expected ${expected}, observed '${revision}' at ${_cs_checkout}")
    endif()
    execute_process(COMMAND "${GIT_EXECUTABLE}" -c "safe.directory=${_cs_checkout}"
        -C "${_cs_checkout}" status --porcelain --untracked-files=all -- include
        RESULT_VARIABLE result OUTPUT_VARIABLE changes OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(NOT result EQUAL 0 OR NOT changes STREQUAL "")
        message(FATAL_ERROR "${label}: include/ differs from the pinned checkout; use a clean separate checkout")
    endif()
endfunction()
