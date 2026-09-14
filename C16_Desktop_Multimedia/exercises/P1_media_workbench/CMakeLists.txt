cmake_minimum_required(VERSION 3.24)
project(c16_p1_media_workbench LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

set(C16_FIXTURES "${C16_COURSE}/build/fixtures")
if(NOT TARGET c16_media_fixtures)
  add_custom_command(
    OUTPUT "${C16_FIXTURES}/c16_pulse_mono_48k_s16.wav" "${C16_FIXTURES}/c16_rgb24_pcm_1s.avi"
    COMMAND "${Python3_EXECUTABLE}" "${C16_COURSE}/tools/make_media_fixtures.py" --output "${C16_FIXTURES}" --self-check
    DEPENDS "${C16_COURSE}/tools/make_media_fixtures.py"
    VERBATIM)
  add_custom_target(c16_media_fixtures DEPENDS "${C16_FIXTURES}/c16_pulse_mono_48k_s16.wav" "${C16_FIXTURES}/c16_rgb24_pcm_1s.avi")
endif()

c16_exercise(c16_p1 QT Core Test EXPECT "check failed:")

add_executable(c16_workbench app/main.cpp app/workbench.cpp app/workbench.hpp)
c16_target(c16_workbench Core Gui Widgets Multimedia MultimediaWidgets Test)
add_dependencies(c16_workbench c16_media_fixtures)
c16_add_test(c16_workbench_smoke c16_workbench ARGS --self-check --fixtures "${C16_FIXTURES}" LABELS app TIMEOUT 20)
