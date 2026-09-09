cmake_minimum_required(VERSION 3.28)
project(C03_Type_Modeling_Interface_Design LANGUAGES CXX)
include(cmake/StudySetup.cmake)

set(lessons L01_invariants L02_value_semantics L03_optional L04_variant L05_expected
    L06_transactions L07_interfaces L08_composition L09_dynamic_polymorphism
    L10_static_polymorphism L11_type_erasure L12_callables L13_indirect_values
    L14_contracts L15_evolution P1_document F01_frontier)
foreach(lesson IN LISTS lessons)
    add_subdirectory(${lesson})
endforeach()
get_property(students GLOBAL PROPERTY C03_STUDENT_TARGETS)
add_custom_target(c03_students DEPENDS ${students})
