cmake_minimum_required(VERSION 3.24)
project(C18_L16_static_check LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)
include(../P2_abi_tool/ClangSupport.cmake)

if(C18_ENABLE_CLANG_TOOLING)
  c18_require_clang_tooling()
  function(c18_l16_variant name path)
    add_executable(c18_l16_${name} "${C18_EXERCISES}/P2_abi_tool/src/l16_driver.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/${path}/solution.cpp")
    c18_target(c18_l16_${name})
    c18_clang_tooling_target(c18_l16_${name})
    c18_add_command_test(c18_l16_${name}_good
      COMMAND "$<TARGET_FILE:c18_l16_${name}>" "${C18_EXERCISES}/P2_abi_tool/fixtures/good/c18_api_ok.hpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
      LABELS "clang;static-check;${name}")
    if(NOT name STREQUAL "student")
      c18_add_command_test(c18_l16_${name}_bad_rejected
        COMMAND "$<TARGET_FILE:c18_l16_${name}>" "${C18_EXERCISES}/P2_abi_tool/fixtures/bad/c18_api_stl.hpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
        EXPECT_FAILURE "dangerous exported ABI type"
        LABELS "clang;static-check;checker")
    endif()
  endfunction()
  if(C18_TEST_STUDENTS)
    c18_l16_variant(student student)
  endif()
  if(C18_BUILD_REFERENCE)
    c18_l16_variant(reference reference)
    c18_l16_variant(good validation/good)
    add_executable(c18_l16_bad "${C18_EXERCISES}/P2_abi_tool/src/l16_driver.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad/solution.cpp")
    c18_target(c18_l16_bad)
    c18_clang_tooling_target(c18_l16_bad)
    c18_add_command_test(c18_l16_bad_missed_declarations
      COMMAND "$<TARGET_FILE:c18_l16_bad>" "${C18_EXERCISES}/P2_abi_tool/fixtures/bad/c18_api_stl.hpp" -- -std=c++23 "-I${C18_EXERCISES}/include"
      EXPECT_FAILURE "run_check: FAIL exit=0"
      LABELS "clang;static-check;checker")
  endif()
endif()
