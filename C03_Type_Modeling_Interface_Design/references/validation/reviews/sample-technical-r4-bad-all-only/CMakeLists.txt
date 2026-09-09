cmake_minimum_required(VERSION 3.28)
project(sample_technical_r4_bad_all_only LANGUAGES CXX)

set(C03_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
set(L06_ROOT "${C03_ROOT}/exercises/L06_transactions")

function(add_l06_all_only target impl_dir)
    add_executable(${target} ../sample-technical-r4-bad-all-only.cpp)
    target_compile_features(${target} PRIVATE cxx_std_23)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
    target_include_directories(${target} PRIVATE
        "${L06_ROOT}/${impl_dir}"
        "${L06_ROOT}/checks"
        "${C03_ROOT}/../C01_Build_Compile_Link/exercises/include")
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /EHsc /W4 /permissive- /Zc:__cplusplus)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_l06_all_only(l06_reference_all_only src/reference)
add_l06_all_only(l06_good_all_only validation/good)
add_l06_all_only(l06_bad_all_only validation/bad)
