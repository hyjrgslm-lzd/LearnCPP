cmake_minimum_required(VERSION 3.28)
project(C02_Objects_Lifetime_Ownership LANGUAGES CXX)
include(cmake/StudySetup.cmake)

foreach(lesson IN ITEMS
    L01_initialization L02_value_categories L03_lifetimes L04_construction
    L05_special_members L06_move_return L07_raii L08_unique L09_shared
    L10_control_block L11_layout L12_storage L13_aliasing L14_ub P1_object_buffer)
    add_subdirectory(${lesson})
endforeach()
