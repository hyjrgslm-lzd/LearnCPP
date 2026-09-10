cmake_minimum_required(VERSION 3.28)
project(L09_dynamic_loading LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

add_library(L09_dynamic_loading_module SHARED checks/module.cpp)
c07_configure_target(L09_dynamic_loading_module)
target_include_directories(L09_dynamic_loading_module PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/checks")
set_target_properties(L09_dynamic_loading_module PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

c07_add_exercise(NAME L09_dynamic_loading HEADER dynamic_loading.hpp CHECK checks/dynamic_loading_checks.cpp
    BAD_DIAGNOSTIC "module call count changed once"
    LIBRARIES ${CMAKE_DL_LIBS}
    ARGS "$<TARGET_FILE:L09_dynamic_loading_module>")

foreach(variant IN ITEMS student reference validation_good validation_bad)
    if(TARGET L09_dynamic_loading_${variant})
        add_dependencies(L09_dynamic_loading_${variant} L09_dynamic_loading_module)
    endif()
endforeach()
