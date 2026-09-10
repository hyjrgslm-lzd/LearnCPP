cmake_minimum_required(VERSION 3.28)
project(C04MetaBlind LANGUAGES CXX)

set(GENERIC_STUDY_ENABLE_META_LIBS ON CACHE BOOL "" FORCE)
include("${CMAKE_CURRENT_LIST_DIR}/../../../../exercises/cmake/StudySetup.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../../../../exercises/cmake/MetaLibraries.cmake")

add_executable(meta_blind_mp11 mp11_blind_check.cpp)
c04_configure_target(meta_blind_mp11)
c04_link_mp11(meta_blind_mp11)
target_include_directories(meta_blind_mp11 PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../exercises/U01_mp11/checks"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../exercises/A02_field_projection/checks")
add_test(NAME meta_blind_mp11 COMMAND meta_blind_mp11)

add_executable(meta_blind_hana hana_blind_check.cpp)
c04_configure_target(meta_blind_hana)
c04_link_hana(meta_blind_hana)
target_include_directories(meta_blind_hana PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../../../exercises/A02_field_projection/checks")
add_test(NAME meta_blind_hana COMMAND meta_blind_hana)
