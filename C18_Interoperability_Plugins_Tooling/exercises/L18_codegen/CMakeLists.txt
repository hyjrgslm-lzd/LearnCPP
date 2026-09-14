cmake_minimum_required(VERSION 3.24)
project(C18_L18_codegen LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)
include(../P2_abi_tool/ClangSupport.cmake)

if(C18_ENABLE_CLANG_TOOLING)
  c18_require_clang_tooling()
  function(c18_l18_variant name path)
    add_executable(c18_l18_${name} "${C18_EXERCISES}/P2_abi_tool/src/l18_driver.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/${path}/solution.cpp")
    c18_target(c18_l18_${name})
    c18_clang_tooling_target(c18_l18_${name})
    c18_add_command_test(c18_l18_${name}_generate
      COMMAND "${Python3_EXECUTABLE}" "${C18_EXERCISES}/P2_abi_tool/verify_codegen.py"
        --out-dir "${CMAKE_CURRENT_BINARY_DIR}/${name}-generated"
        --cc "${CMAKE_C_COMPILER}" --cxx "${CMAKE_CXX_COMPILER}" --include "${C18_EXERCISES}/include"
        "$<TARGET_FILE:c18_l18_${name}>" "${C18_EXERCISES}/include/c18/abi.h" -- -std=c++23 "-I${C18_EXERCISES}/include"
      LABELS "clang;codegen;${name}")
    if(NOT name STREQUAL "student")
      c18_add_command_test(c18_l18_${name}_existing_output_rejected
        COMMAND "${Python3_EXECUTABLE}" "${C18_EXERCISES}/P2_abi_tool/verify_codegen.py"
          --expect-failure "output already exists" --precreate "exports.txt"
          --out-dir "${CMAKE_CURRENT_BINARY_DIR}/${name}-existing"
          --cc "${CMAKE_C_COMPILER}" --cxx "${CMAKE_CXX_COMPILER}" --include "${C18_EXERCISES}/include"
          "$<TARGET_FILE:c18_l18_${name}>" "${C18_EXERCISES}/include/c18/abi.h" -- -std=c++23 "-I${C18_EXERCISES}/include"
        LABELS "clang;codegen;output-guard;checker")
    endif()
  endfunction()
  if(C18_TEST_STUDENTS)
    c18_l18_variant(student student)
  endif()
  if(C18_BUILD_REFERENCE)
    c18_l18_variant(reference reference)
    c18_l18_variant(good validation/good)
    add_executable(c18_l18_bad "${C18_EXERCISES}/P2_abi_tool/src/l18_driver.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad/solution.cpp")
    c18_target(c18_l18_bad)
    c18_clang_tooling_target(c18_l18_bad)
    c18_add_command_test(c18_l18_bad_rejected
      COMMAND "${Python3_EXECUTABLE}" "${C18_EXERCISES}/P2_abi_tool/verify_codegen.py"
        --expect-failure "generator missed manifest or contract files"
        --out-dir "${CMAKE_CURRENT_BINARY_DIR}/bad-generated"
        --cc "${CMAKE_C_COMPILER}" --cxx "${CMAKE_CXX_COMPILER}" --include "${C18_EXERCISES}/include"
        "$<TARGET_FILE:c18_l18_bad>" "${C18_EXERCISES}/include/c18/abi.h" -- -std=c++23 "-I${C18_EXERCISES}/include"
      LABELS "clang;codegen;checker")
  endif()
endif()
