cmake_minimum_required(VERSION 3.24)
project(c16_u01_quick_bridge LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
set(CMAKE_AUTORCC ON)

set(C16_FIXTURES "${C16_COURSE}/build/fixtures")
if(NOT TARGET c16_media_fixtures)
  add_custom_command(
    OUTPUT "${C16_FIXTURES}/c16_pulse_mono_48k_s16.wav" "${C16_FIXTURES}/c16_rgb24_pcm_1s.avi"
    COMMAND "${Python3_EXECUTABLE}" "${C16_COURSE}/tools/make_media_fixtures.py" --output "${C16_FIXTURES}" --self-check
    DEPENDS "${C16_COURSE}/tools/make_media_fixtures.py"
    VERBATIM)
  add_custom_target(c16_media_fixtures DEPENDS "${C16_FIXTURES}/c16_pulse_mono_48k_s16.wav" "${C16_FIXTURES}/c16_rgb24_pcm_1s.avi")
endif()

c16_exercise(c16_u01 QT Core Test EXPECT "check failed:")

add_executable(c16_quick quick_main.cpp quick_bridge.qrc
  ../P1_media_workbench/app/workbench.cpp
  ../P1_media_workbench/app/workbench.hpp
  Main.qml)
c16_target(c16_quick Core Gui Widgets Multimedia MultimediaWidgets Qml Quick QuickControls2 Test)
target_include_directories(c16_quick PRIVATE ../P1_media_workbench/app)
add_dependencies(c16_quick c16_media_fixtures)
c16_add_test(c16_quick_smoke c16_quick ARGS --self-check --fixtures "${C16_FIXTURES}" LABELS quick TIMEOUT 20)
