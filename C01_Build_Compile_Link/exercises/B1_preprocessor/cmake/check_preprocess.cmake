if(NOT DEFINED SOURCE_DIR OR NOT DEFINED BINARY_DIR OR NOT DEFINED CXX_COMPILER OR NOT DEFINED COMPILER_ID)
    message(FATAL_ERROR "SOURCE_DIR, BINARY_DIR, CXX_COMPILER, and COMPILER_ID are required")
endif()

file(MAKE_DIRECTORY "${BINARY_DIR}/evidence")

function(_preprocess source output out_var)
    if(COMPILER_ID STREQUAL "MSVC")
        execute_process(
            COMMAND "${CXX_COMPILER}" /nologo /std:c++latest /Zc:preprocessor /EP /I"${SOURCE_DIR}/src/reference" "${SOURCE_DIR}/src/reference/${source}"
            RESULT_VARIABLE result
            OUTPUT_VARIABLE out
            ERROR_VARIABLE err
        )
    else()
        execute_process(
            COMMAND "${CXX_COMPILER}" -std=c++23 -E -I"${SOURCE_DIR}/src/reference" "${SOURCE_DIR}/src/reference/${source}"
            RESULT_VARIABLE result
            OUTPUT_VARIABLE out
            ERROR_VARIABLE err
        )
    endif()
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "preprocess failed for ${source}:\n${out}\n${err}")
    endif()
    file(WRITE "${BINARY_DIR}/evidence/${output}" "${out}\n${err}")
    string(REGEX REPLACE "[ \t\r\n]+" " " normalized "${out}")
    set(${out_var} "${normalized}" PARENT_SCOPE)
endfunction()

_preprocess("macro_a.cpp" "preprocess_macro_a.txt" macro_a_text)
_preprocess("macro_b.cpp" "preprocess_macro_b.txt" macro_b_text)

set(macro_a_regex [=[static inline int generated_value[(][)] [{] return [(][(]100[)] [+] 11[)] *; [}]]=])
set(macro_b_regex [=[static inline int generated_value[(][)] [{] return [(][(]100[)] [+] 22[)] *; [}]]=])

if(NOT macro_a_text MATCHES "${macro_a_regex}")
    message(FATAL_ERROR "macro_a preprocess output does not show generated_value returning ((100) + 11):\n${macro_a_text}")
endif()
if(NOT macro_b_text MATCHES "${macro_b_regex}")
    message(FATAL_ERROR "macro_b preprocess output does not show generated_value returning ((100) + 22):\n${macro_b_text}")
endif()
message(STATUS "preprocess evidence shows generated_value has separate internal-linkage definitions with different macro-expanded returns")
