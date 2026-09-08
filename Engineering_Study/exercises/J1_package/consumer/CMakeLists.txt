cmake_minimum_required(VERSION 3.28)

project(J1_package_consumer LANGUAGES CXX)

set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON)
set(CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY ON)
set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY FALSE)

set(REQUESTED_VERSION "1.0.0" CACHE STRING "LessonPackage version to request")
set(LESSON_TARGET "lesson_static" CACHE STRING "lesson_static, lesson_shared, or missing")

find_package(LessonPackage ${REQUESTED_VERSION} CONFIG REQUIRED)

add_executable(package_consumer main.cpp)
target_compile_features(package_consumer PRIVATE cxx_std_23)
set_target_properties(package_consumer PROPERTIES CXX_EXTENSIONS OFF)

if(LESSON_TARGET STREQUAL "lesson_static")
    target_link_libraries(package_consumer PRIVATE LessonPackage::lesson_static)
elseif(LESSON_TARGET STREQUAL "lesson_shared")
    target_link_libraries(package_consumer PRIVATE LessonPackage::lesson_shared)
elseif(LESSON_TARGET STREQUAL "missing")
    target_link_libraries(package_consumer PRIVATE LessonPackage::lesson)
else()
    message(FATAL_ERROR "unknown LESSON_TARGET=${LESSON_TARGET}")
endif()
