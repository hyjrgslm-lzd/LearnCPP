cmake_minimum_required(VERSION 3.25)
project(C12_L08_postgres_client LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
if(C12_ENABLE_POSTGRES AND C12_BUILD_REFERENCE)
  add_executable(C12_L08_pg main.cpp)
  c12_target(C12_L08_pg)
  target_link_libraries(C12_L08_pg PRIVATE c12_pq)
  if(WIN32)
    target_link_libraries(C12_L08_pg PRIVATE ws2_32)
  endif()
  foreach(case client isolation cancel)
    c12_test(C12_L08_${case} COMMAND "${Python3_EXECUTABLE}" "${C12_EXERCISES}/tools/postgres_fixture.py"
      --root "${C12_POSTGRES_ROOT}" --client "$<TARGET_FILE:C12_L08_pg>" --case ${case} TIMEOUT 240 LABELS postgres observation)
  endforeach()
  c12_test(C12_L08_environment COMMAND "${Python3_EXECUTABLE}" "${C12_EXERCISES}/tools/postgres_fixture.py"
    --root "${C12_POSTGRES_ROOT}" TIMEOUT 150 LABELS helper postgres)
  set_tests_properties(C12_L08_environment PROPERTIES
    ENVIRONMENT "PGHOSTADDR=127.0.0.2;PGSERVICE=c12_nonexistent_service;PGOPTIONS=-c statement_timeout=1")
endif()
