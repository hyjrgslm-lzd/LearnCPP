cmake_minimum_required(VERSION 3.28)
project(F01_frontier LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
include(CheckCXXSourceCompiles)

function(c05_frontier_probe name source)
    add_executable(${name} ${source})
    c05_configure_target(${name})
    if(MSVC)
        target_compile_options(${name} PRIVATE /std:c++latest)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${name} PRIVATE -std=c++2c)
    endif()
    c05_add_test(${name} LABELS capability)
    set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77)
endfunction()

if(NOT DATA_STUDY_ENABLE_FRONTIER)
    message(STATUS "F01 disabled; no capability tests registered")
    return()
endif()

c05_frontier_probe(F01_text_encoding_locale text_encoding_locale.cpp)
c05_frontier_probe(F01_charconv_result_bool charconv_result_bool.cpp)
c05_frontier_probe(F01_to_string_semantics to_string_semantics.cpp)
c05_frontier_probe(F01_runtime_format runtime_format.cpp)
c05_frontier_probe(F01_path_formatter path_formatter.cpp)
c05_frontier_probe(F01_format_float_c29 format_float_c29.cpp)
c05_frontier_probe(F01_error_code_formatter_c29 error_code_formatter_c29.cpp)
if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "/std:c++latest")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_REQUIRED_FLAGS "-std=c++2c")
endif()
check_cxx_source_compiles("
    #include <format>
    #include <string>
    int main() {
        std::string text = std::format(\"{}\", 42);
        return text == \"42\" ? 0 : 1;
    }
" C05_HAS_BASIC_FORMAT)
if(NOT C05_HAS_BASIC_FORMAT)
    message(FATAL_ERROR "Basic std::format probe failed; cannot classify C++29 formatter capabilities")
endif()
check_cxx_source_compiles("
    #include <format>
    #include <system_error>
    int main() {
        std::error_code ec = std::make_error_code(std::errc::invalid_argument);
        (void) std::format(\"{}\", ec);
    }
" C05_HAS_ERROR_CODE_FORMATTER)
unset(CMAKE_REQUIRED_FLAGS)
if(C05_HAS_ERROR_CODE_FORMATTER)
    target_compile_definitions(F01_error_code_formatter_c29 PRIVATE C05_HAS_ERROR_CODE_FORMATTER=1)
endif()
c05_frontier_probe(F01_bitops_shift_c29 bitops_shift_c29.cpp)
c05_frontier_probe(F01_bitops_permutation_c29 bitops_permutation_c29.cpp)
