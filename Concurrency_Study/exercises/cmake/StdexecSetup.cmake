include_guard(GLOBAL)
if(TARGET stdexec::stdexec)
    return()
endif()
include("${CMAKE_CURRENT_LIST_DIR}/DependencyRevision.cmake")
set(CONCURRENCY_STUDY_STDEXEC_REVISION "6d7ad689f4d4831c5136e4abe1c601f9a3b64e43" CACHE INTERNAL "stdexec nvhpc-26.05 commit" FORCE)
set(CONCURRENCY_STUDY_STDEXEC_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../third_party/stdexec" CACHE PATH "Existing stdexec nvhpc-26.05 checkout")
if(EXISTS "${CONCURRENCY_STUDY_STDEXEC_SOURCE_DIR}/include/stdexec/execution.hpp")
    cs_verify_checkout("${CONCURRENCY_STUDY_STDEXEC_SOURCE_DIR}" "${CONCURRENCY_STUDY_STDEXEC_REVISION}" stdexec)
    set(_cs_stdexec_include "${CONCURRENCY_STUDY_STDEXEC_SOURCE_DIR}/include")
else()
    if(NOT CONCURRENCY_STUDY_FETCH_DEPS)
        message(FATAL_ERROR "stdexec nvhpc-26.05 is not available. Set CONCURRENCY_STUDY_STDEXEC_SOURCE_DIR or explicitly enable CONCURRENCY_STUDY_FETCH_DEPS.")
    endif()
    include(FetchContent)
    FetchContent_Declare(stdexec
        GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
        GIT_TAG 6d7ad689f4d4831c5136e4abe1c601f9a3b64e43
        SOURCE_SUBDIR _cs_headers_only)
    FetchContent_MakeAvailable(stdexec)
    cs_verify_checkout("${stdexec_SOURCE_DIR}" "6d7ad689f4d4831c5136e4abe1c601f9a3b64e43" stdexec)
    set(_cs_stdexec_include "${stdexec_SOURCE_DIR}/include")
endif()
# These exercises use stdexec's header-only facilities, not its optional
# separately compiled runtime libraries.
add_library(cs_stdexec_headers INTERFACE)
target_include_directories(cs_stdexec_headers INTERFACE "${_cs_stdexec_include}")
target_compile_features(cs_stdexec_headers INTERFACE cxx_std_20)
target_compile_options(cs_stdexec_headers INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>)
add_library(stdexec::stdexec ALIAS cs_stdexec_headers)
message(STATUS "[study] stdexec headers: ${_cs_stdexec_include}; expected source version: nvhpc-26.05")
