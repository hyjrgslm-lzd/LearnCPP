cmake_minimum_required(VERSION 3.24)
project(C18_L15_ast LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)

function(c18_l15_variant variant implementation)
  c18_add_command_test(c18_l15_${variant}
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" "${CMAKE_CURRENT_SOURCE_DIR}/${implementation}/solution.py"
    LABELS "clang;ast;${variant}" ALLOW_SKIP)
endfunction()

if(C18_TEST_STUDENTS)
  c18_l15_variant(student student)
endif()
if(C18_BUILD_REFERENCE)
  c18_l15_variant(reference reference)
  c18_l15_variant(good validation/good)
  c18_add_command_test(c18_l15_bad_rejected
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/checks.py" --expect-reject "${CMAKE_CURRENT_SOURCE_DIR}/validation/bad/solution.py"
    LABELS "clang;ast;checker" ALLOW_SKIP)
endif()
