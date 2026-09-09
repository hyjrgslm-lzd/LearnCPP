cmake_minimum_required(VERSION 3.28)
project(c02_0810_shadow_fixture LANGUAGES CXX)
set(ROOT "F:/CPPTrain/LearnCPP")
function(add_shadow name lesson fake impl header)
    add_executable(${name} "${ROOT}/Core_Study/exercises/${lesson}/checks/validation_check.cpp")
    target_compile_features(${name} PRIVATE cxx_std_23)
    target_include_directories(${name} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/${fake}"
        "${ROOT}/Core_Study/exercises/${lesson}/validation/good"
        "${ROOT}/Core_Study/exercises/${lesson}/checks"
        "${ROOT}/Engineering_Study/exercises/include")
    set_target_properties(${name} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
endfunction()
add_shadow(shadow_l08 L08_unique fake_l08 owner.hpp owner)
add_shadow(shadow_l09 L09_shared fake_l09 owner.hpp owner)
add_shadow(shadow_l10 L10_control_block fake_l10 rc.hpp rc)
