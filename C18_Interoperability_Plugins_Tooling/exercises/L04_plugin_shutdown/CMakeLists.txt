cmake_minimum_required(VERSION 3.24)
project(C18_L04_plugin_shutdown LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)

add_library(c18_l04_good_plugin SHARED fixtures/good_plugin.cpp)
c18_target(c18_l04_good_plugin)
target_compile_definitions(c18_l04_good_plugin PRIVATE C18_PLUGIN_BUILD)
set_target_properties(c18_l04_good_plugin PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

add_library(c18_l04_busy_destroy_plugin SHARED fixtures/good_plugin.cpp)
c18_target(c18_l04_busy_destroy_plugin)
target_compile_definitions(c18_l04_busy_destroy_plugin PRIVATE C18_PLUGIN_BUILD C18_L04_BUSY_DESTROY)
set_target_properties(c18_l04_busy_destroy_plugin PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

add_library(c18_l04_missing_symbol SHARED fixtures/missing_symbol.cpp)
c18_target(c18_l04_missing_symbol)
target_compile_definitions(c18_l04_missing_symbol PRIVATE C18_PLUGIN_BUILD)
set_target_properties(c18_l04_missing_symbol PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

add_library(c18_l04_version_mismatch SHARED fixtures/version_mismatch.cpp)
c18_target(c18_l04_version_mismatch)
target_compile_definitions(c18_l04_version_mismatch PRIVATE C18_PLUGIN_BUILD)
set_target_properties(c18_l04_version_mismatch PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

add_library(c18_l04_throwing_entry SHARED fixtures/throwing_entry.cpp)
c18_target(c18_l04_throwing_entry)
target_compile_definitions(c18_l04_throwing_entry PRIVATE C18_PLUGIN_BUILD)
set_target_properties(c18_l04_throwing_entry PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

function(c18_l04_variant variant implementation)
  set(target "c18_l04_${variant}")
  add_executable(${target} checks.cpp)
  c18_target(${target})
  target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${implementation}")
  target_link_libraries(${target} PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
  add_dependencies(${target}
    c18_l04_good_plugin
    c18_l04_busy_destroy_plugin
    c18_l04_missing_symbol
    c18_l04_version_mismatch
    c18_l04_throwing_entry)
endfunction()

c18_l04_variant(student student)
if(C18_BUILD_REFERENCE)
  c18_l04_variant(reference reference)
  c18_l04_variant(good validation/good)
  c18_l04_variant(bad validation/bad)
endif()

set(l04_args
  "$<TARGET_FILE:c18_l04_good_plugin>"
  "$<TARGET_FILE:c18_l04_busy_destroy_plugin>"
  "$<TARGET_FILE:c18_l04_missing_symbol>"
  "$<TARGET_FILE:c18_l04_version_mismatch>"
  "$<TARGET_FILE:c18_l04_throwing_entry>")

if(C18_BUILD_REFERENCE)
  c18_add_test(c18_l04_reference c18_l04_reference ARGS ${l04_args} LABELS "reference;plugin")
  c18_add_test(c18_l04_good c18_l04_good ARGS ${l04_args} LABELS "good;plugin")
  c18_add_test(c18_l04_bad_rejected c18_l04_bad ARGS ${l04_args}
    EXPECT_FAILURE "check failed: close returned before callback drained" LABELS "checker;plugin")
endif()
if(C18_TEST_STUDENTS)
  c18_add_test(c18_l04_student c18_l04_student ARGS ${l04_args} LABELS "student;plugin")
endif()
