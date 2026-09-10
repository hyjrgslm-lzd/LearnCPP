cmake_minimum_required(VERSION 3.28)
project(C04_Generic_CompileTime_Reflection LANGUAGES CXX)
include(cmake/StudySetup.cmake)
set(lessons L01_templates L02_deduction L03_forwarding L04_lookup L05_overload
    L06_constraints L07_packs L08_type_lists L09_tuple L10_constexpr
    L11_customization B01_compile_cost P1_static_record F01_frontier
    A01_compiletime_values A02_field_projection A03_explicit_object
    A04_type_pipelines A05_expression_templates U01_mp11 U02_hana)
foreach(lesson IN LISTS lessons)
    add_subdirectory(${lesson})
endforeach()
get_property(students GLOBAL PROPERTY C04_STUDENT_TARGETS)
add_custom_target(c04_students DEPENDS ${students})
c04_add_test(C04_diagnostic_message_controls COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_diagnostic_messages.py" LABELS validation)
