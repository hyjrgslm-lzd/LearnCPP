set(GENERIC_STUDY_ENABLE_META_LIBS ON CACHE BOOL "" FORCE)
include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/MetaLibraries.cmake")
foreach(_target control subject)
    target_include_directories(${_target} PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../../validation/good"
        "${CMAKE_CURRENT_LIST_DIR}/../../checks"
        "${CMAKE_CURRENT_LIST_DIR}/../../../A02_field_projection/checks")
    c04_link_mp11(${_target})
endforeach()
