cmake_minimum_required(VERSION 3.24)
project(C18_L09_limited_api LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)
include(../backends/PythonSupport.cmake)

set(c18_l09_inspector_args)
if(WIN32 AND C18_ENABLE_LIMITED_API)
  if(MSVC)
    get_filename_component(c18_l09_compiler_bin "${CMAKE_LINKER}" DIRECTORY)
    set(c18_l09_imports_tool "${c18_l09_compiler_bin}/dumpbin.exe")
  else()
    set(c18_l09_imports_tool "${CMAKE_OBJDUMP}")
  endif()
  if(NOT EXISTS "${c18_l09_imports_tool}")
    message(FATAL_ERROR "L09 needs an existing dumpbin or objdump to inspect real PE imports")
  endif()
  list(APPEND c18_l09_inspector_args --imports-tool "${c18_l09_imports_tool}")
endif()

function(c18_l09_limited variant implementation)
  if(NOT C18_ENABLE_LIMITED_API)
    return()
  endif()
  c18_limited_extension(c18_l09_${variant} "${implementation}/solution.cpp")
  target_compile_definitions(c18_l09_${variant} PRIVATE C18_MODULE_NAME=c18_l09_${variant})
  set(c18_l09_${variant}_PY38_EXECUTABLE "${c18_l09_${variant}_PY38_EXECUTABLE}" PARENT_SCOPE)
endfunction()

c18_l09_limited(student student)
if(C18_BUILD_REFERENCE)
  c18_l09_limited(reference reference)
  c18_l09_limited(good validation/good)
  if(C18_ENABLE_LIMITED_API AND WIN32)
    c18_python38_paths(c18_py38_exe c18_py38_include c18_py38_libdir)
    add_library(c18_l09_bad MODULE validation/bad/solution.cpp)
    c18_target(c18_l09_bad)
    target_compile_definitions(c18_l09_bad PRIVATE C18_MODULE_NAME=c18_l09_bad)
    target_include_directories(c18_l09_bad PRIVATE "${c18_py38_include}")
    if(WIN32)
      target_link_libraries(c18_l09_bad PRIVATE "${c18_py38_libdir}/python38.lib")
      set_target_properties(c18_l09_bad PROPERTIES PREFIX "" SUFFIX ".pyd")
    endif()
  endif()
endif()

if(C18_ENABLE_LIMITED_API AND C18_BUILD_REFERENCE)
  c18_add_command_test(c18_l09_reference_py38
    COMMAND "${c18_l09_reference_PY38_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l09_reference>" ${c18_l09_inspector_args}
    LABELS "reference;python;limited")
  c18_add_command_test(c18_l09_reference_py310
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l09_reference>" ${c18_l09_inspector_args}
    LABELS "reference;python;limited")
  c18_add_command_test(c18_l09_good_py38
    COMMAND "${c18_l09_good_PY38_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l09_good>" ${c18_l09_inspector_args}
    LABELS "good;python;limited")
  c18_add_command_test(c18_l09_good_py310
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l09_good>" ${c18_l09_inspector_args}
    LABELS "good;python;limited")
  if(WIN32)
    c18_add_command_test(c18_l09_bad_rejected
      COMMAND "${c18_py38_exe}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l09_bad>" ${c18_l09_inspector_args}
      EXPECT_FAILURE "check failed: extension depends on a version-specific Python DLL" LABELS "checker;python;limited")
  endif()
endif()
if(C18_ENABLE_LIMITED_API AND C18_TEST_STUDENTS)
  c18_add_command_test(c18_l09_student
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "$<TARGET_FILE:c18_l09_student>" ${c18_l09_inspector_args}
    LABELS "student;python;limited")
endif()
