cmake_minimum_required(VERSION 3.24)
project(c16_l14 LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${C16_COURSE}/tools/make_media_fixtures.py" --output "${C16_COURSE}/build/fixtures"
  RESULT_VARIABLE c16_fixture_result
)
if(NOT c16_fixture_result EQUAL 0)
  message(FATAL_ERROR "failed to generate C16 media fixtures")
endif()

c16_exercise(c16_l14 QT Core Gui Multimedia Test EXPECT "check failed:")

add_executable(c16_audio_device device_audio.cpp)
c16_target(c16_audio_device Core Multimedia)
c16_add_test(c16_audio_device c16_audio_device ALLOW_SKIP LABELS device TIMEOUT 10)
