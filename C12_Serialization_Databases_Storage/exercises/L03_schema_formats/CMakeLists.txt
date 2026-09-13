cmake_minimum_required(VERSION 3.25)
project(C12_L03_schema_formats LANGUAGES C CXX)
include(../cmake/StudySetup.cmake)
if(C12_ENABLE_FORMATS AND C12_BUILD_REFERENCE)
  set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
  file(MAKE_DIRECTORY "${generated}" "${CMAKE_CURRENT_BINARY_DIR}/rejected")
  foreach(version v1 v2)
    add_custom_command(OUTPUT "${generated}/manifest_${version}.pb.cc" "${generated}/manifest_${version}.pb.h"
      COMMAND "$<TARGET_FILE:protoc>" --proto_path "${CMAKE_CURRENT_SOURCE_DIR}" --cpp_out "${generated}"
        "${CMAKE_CURRENT_SOURCE_DIR}/manifest_${version}.proto"
      DEPENDS protoc "${CMAKE_CURRENT_SOURCE_DIR}/manifest_${version}.proto" VERBATIM)
    list(APPEND generated_sources "${generated}/manifest_${version}.pb.cc")
  endforeach()
  add_library(c12_formats STATIC ${generated_sources})
  target_compile_features(c12_formats PUBLIC cxx_std_23)
  target_include_directories(c12_formats PUBLIC "${generated}" "${C12_EXERCISES}/include"
    "${C12_REPO}/C05_Data_Representation_Standard_Facilities/exercises/include")
  target_link_libraries(c12_formats PUBLIC libprotobuf c12_json)
  c12_observation(C12_L03 main.cpp)
  target_link_libraries(C12_L03 PRIVATE c12_formats)
  c12_test(C12_L03_reserved COMMAND "$<TARGET_FILE:protoc>" --proto_path "${CMAKE_CURRENT_SOURCE_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}/rejected" "${CMAKE_CURRENT_SOURCE_DIR}/reserved_bad.proto"
    EXIT 1 TEXT "uses reserved number" LABELS checker)
endif()
