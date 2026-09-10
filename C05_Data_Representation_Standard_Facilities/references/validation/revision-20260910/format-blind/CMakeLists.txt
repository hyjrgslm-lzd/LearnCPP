cmake_minimum_required(VERSION 3.25)
project(format_blind_review LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DATA_STUDY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
set(FMT_SOURCE "${DATA_STUDY_ROOT}/exercises/build/_deps/fmt-12.1.0/source")
set(SPDLOG_SOURCE "${DATA_STUDY_ROOT}/exercises/build/_deps/spdlog-1.17.0/source")

add_subdirectory("${FMT_SOURCE}" "${CMAKE_BINARY_DIR}/fmt-build" EXCLUDE_FROM_ALL)

add_library(blind_spdlog_headers INTERFACE)
target_include_directories(blind_spdlog_headers INTERFACE "${SPDLOG_SOURCE}/include")

add_executable(blind_fmt_probe fmt_probe.cpp)
target_link_libraries(blind_fmt_probe PRIVATE fmt::fmt)

add_executable(blind_bad_fmt_compile EXCLUDE_FROM_ALL bad_fmt_compile.cpp)
target_link_libraries(blind_bad_fmt_compile PRIVATE fmt::fmt)

function(add_spdlog_probe target backend active_level)
  add_executable(${target} spdlog_probe.cpp)
  target_link_libraries(${target} PRIVATE blind_spdlog_headers)
  target_compile_definitions(${target} PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${active_level})
  if(backend STREQUAL "fmt")
    target_link_libraries(${target} PRIVATE fmt::fmt)
    target_compile_definitions(${target} PRIVATE SPDLOG_FMT_EXTERNAL BLIND_USE_FMT_BACKEND)
  elseif(backend STREQUAL "std")
    target_compile_definitions(${target} PRIVATE SPDLOG_USE_STD_FORMAT BLIND_USE_STD_BACKEND)
  else()
    message(FATAL_ERROR "unknown backend: ${backend}")
  endif()
endfunction()

add_spdlog_probe(blind_spdlog_fmt_debug fmt DEBUG)
add_spdlog_probe(blind_spdlog_fmt_info fmt INFO)
add_spdlog_probe(blind_spdlog_std_debug std DEBUG)
add_spdlog_probe(blind_spdlog_std_info std INFO)

add_executable(blind_bad_spdlog_compile EXCLUDE_FROM_ALL bad_spdlog_compile.cpp)
target_link_libraries(blind_bad_spdlog_compile PRIVATE blind_spdlog_headers fmt::fmt)
target_compile_definitions(blind_bad_spdlog_compile PRIVATE SPDLOG_FMT_EXTERNAL SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
