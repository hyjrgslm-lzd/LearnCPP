cmake_minimum_required(VERSION 3.24)
project(C18_P2_abi_tool LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(../cmake/StudySetup.cmake)
include(ClangSupport.cmake)

add_executable(c18_output_guard_self_check src/output_guard_self_check.cpp)
c18_target(c18_output_guard_self_check)
c18_add_test(c18_output_guard_self_check c18_output_guard_self_check LABELS "infrastructure;output-guard")

if(C18_ENABLE_CLANG_TOOLING)
  c18_require_clang_tooling()
  add_executable(c18_tool src/c18_tool.cpp)
  c18_target(c18_tool)
  c18_clang_tooling_target(c18_tool)
  c18_add_command_test(c18_p2_check_good
    COMMAND "$<TARGET_FILE:c18_tool>" --c18-subcommand=check
      "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/good/c18_api_ok.hpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
    LABELS "clang;tooling;check")
  c18_add_command_test(c18_p2_check_bad_rejected
    COMMAND "$<TARGET_FILE:c18_tool>" --c18-subcommand=check
      "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bad/c18_api_stl.hpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
    EXPECT_FAILURE "dangerous exported ABI type"
    LABELS "clang;tooling;check")
  c18_add_command_test(c18_p2_rewrite_plain
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_rewrite.py" --mode plain
      --out-dir "${CMAKE_CURRENT_BINARY_DIR}/rewrite-out"
      "$<TARGET_FILE:c18_tool>" "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rewrite/use_old_symbol.cpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
    LABELS "clang;tooling;rewrite")
  c18_add_command_test(c18_p2_rewrite_macro_rejected
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_rewrite.py" --mode reject
      --expect "refuse macro/template/system-header rewrite"
      --out-dir "${CMAKE_CURRENT_BINARY_DIR}/rewrite-refused"
      "$<TARGET_FILE:c18_tool>" "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rewrite/macro_refused.cpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
    LABELS "clang;tooling;rewrite")
  c18_add_command_test(c18_p2_rewrite_template_rejected
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_rewrite.py" --mode reject
      --expect "refuse macro/template/system-header rewrite"
      --out-dir "${CMAKE_CURRENT_BINARY_DIR}/rewrite-template-refused"
      "$<TARGET_FILE:c18_tool>" "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rewrite/template_refused.cpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
    LABELS "clang;tooling;rewrite")
  c18_add_command_test(c18_p2_rewrite_input_dir_rejected
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_rewrite.py" --mode guard-input
      --out-dir "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rewrite"
      "$<TARGET_FILE:c18_tool>" "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rewrite/use_old_symbol.cpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
    LABELS "clang;tooling;rewrite;output-guard")
  c18_add_command_test(c18_p2_rewrite_existing_rejected
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_rewrite.py" --mode existing-output
      --out-dir "${CMAKE_CURRENT_BINARY_DIR}/rewrite-existing"
      "$<TARGET_FILE:c18_tool>" "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rewrite/use_old_symbol.cpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
    LABELS "clang;tooling;rewrite;output-guard")
  c18_add_command_test(c18_p2_generate_contract
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_codegen.py"
      --out-dir "${CMAKE_CURRENT_BINARY_DIR}/generated"
      --cc "${CMAKE_C_COMPILER}" --cxx "${CMAKE_CXX_COMPILER}" --include "${C18_EXERCISES}/include"
      "$<TARGET_FILE:c18_tool>" "${C18_EXERCISES}/include/c18/abi.h" -- -std=c++23 "-I${C18_EXERCISES}/include"
    LABELS "clang;tooling;generate")
  c18_add_command_test(c18_p2_generate_existing_rejected
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_codegen.py"
      --expect-failure "output already exists" --precreate "exports.txt"
      --out-dir "${CMAKE_CURRENT_BINARY_DIR}/generated-existing"
      --cc "${CMAKE_C_COMPILER}" --cxx "${CMAKE_CXX_COMPILER}" --include "${C18_EXERCISES}/include"
      "$<TARGET_FILE:c18_tool>" "${C18_EXERCISES}/include/c18/abi.h" -- -std=c++23 "-I${C18_EXERCISES}/include"
    LABELS "clang;tooling;generate;output-guard")
endif()
