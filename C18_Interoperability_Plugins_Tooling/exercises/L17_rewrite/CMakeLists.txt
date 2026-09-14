cmake_minimum_required(VERSION 3.24)
project(C18_L17_rewrite LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)
include(../P2_abi_tool/ClangSupport.cmake)

if(C18_ENABLE_CLANG_TOOLING)
  c18_require_clang_tooling()
  function(c18_l17_variant name path)
    add_executable(c18_l17_${name} "${C18_EXERCISES}/P2_abi_tool/src/l17_driver.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/${path}/solution.cpp")
    c18_target(c18_l17_${name})
    c18_clang_tooling_target(c18_l17_${name})
    c18_add_command_test(c18_l17_${name}_plain
      COMMAND "${Python3_EXECUTABLE}" "${C18_EXERCISES}/P2_abi_tool/verify_rewrite.py" --mode plain
        --out-dir "${CMAKE_CURRENT_BINARY_DIR}/${name}-rewrite"
        "$<TARGET_FILE:c18_l17_${name}>" "${C18_EXERCISES}/P2_abi_tool/fixtures/rewrite/use_old_symbol.cpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
      LABELS "clang;rewrite;${name}")
    if(NOT name STREQUAL "student")
      c18_add_command_test(c18_l17_${name}_macro_rejected
        COMMAND "${Python3_EXECUTABLE}" "${C18_EXERCISES}/P2_abi_tool/verify_rewrite.py" --mode reject --expect "refuse macro/template/system-header rewrite"
          --out-dir "${CMAKE_CURRENT_BINARY_DIR}/${name}-macro"
          "$<TARGET_FILE:c18_l17_${name}>" "${C18_EXERCISES}/P2_abi_tool/fixtures/rewrite/macro_refused.cpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
        LABELS "clang;rewrite;checker")
      c18_add_command_test(c18_l17_${name}_template_rejected
        COMMAND "${Python3_EXECUTABLE}" "${C18_EXERCISES}/P2_abi_tool/verify_rewrite.py" --mode reject --expect "refuse macro/template/system-header rewrite"
          --out-dir "${CMAKE_CURRENT_BINARY_DIR}/${name}-template"
          "$<TARGET_FILE:c18_l17_${name}>" "${C18_EXERCISES}/P2_abi_tool/fixtures/rewrite/template_refused.cpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
        LABELS "clang;rewrite;checker")
      c18_add_command_test(c18_l17_${name}_existing_output_rejected
        COMMAND "${Python3_EXECUTABLE}" "${C18_EXERCISES}/P2_abi_tool/verify_rewrite.py" --mode existing-output
          --out-dir "${CMAKE_CURRENT_BINARY_DIR}/${name}-existing"
          "$<TARGET_FILE:c18_l17_${name}>" "${C18_EXERCISES}/P2_abi_tool/fixtures/rewrite/use_old_symbol.cpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
        LABELS "clang;rewrite;output-guard;checker")
    endif()
  endfunction()
  if(C18_TEST_STUDENTS)
    c18_l17_variant(student student)
  endif()
  if(C18_BUILD_REFERENCE)
    c18_l17_variant(reference reference)
    c18_l17_variant(good validation/good)
    add_executable(c18_l17_bad "${C18_EXERCISES}/P2_abi_tool/src/l17_driver.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad/solution.cpp")
    c18_target(c18_l17_bad)
    c18_clang_tooling_target(c18_l17_bad)
    c18_add_command_test(c18_l17_bad_macro_not_refused
      COMMAND "${Python3_EXECUTABLE}" "${C18_EXERCISES}/P2_abi_tool/verify_rewrite.py" --mode reject --expect "refuse macro/template/system-header rewrite"
        --out-dir "${CMAKE_CURRENT_BINARY_DIR}/bad-macro"
        "$<TARGET_FILE:c18_l17_bad>" "${C18_EXERCISES}/P2_abi_tool/fixtures/rewrite/macro_refused.cpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
      EXPECT_FAILURE "rewrite native tool result did not match expectation"
      LABELS "clang;rewrite;checker")
  endif()
endif()
