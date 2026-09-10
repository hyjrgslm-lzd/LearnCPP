cmake_minimum_required(VERSION 3.28)
project(L11_calendar_zones LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c05_add_observation(L11_calendar_zones_utc utc_observation.cpp)
add_executable(L11_calendar_zones_tzdb tzdb_observation.cpp)
c05_configure_target(L11_calendar_zones_tzdb)
c05_add_test(L11_calendar_zones_tzdb LABELS capability)
set_tests_properties(L11_calendar_zones_tzdb PROPERTIES SKIP_RETURN_CODE 77)
