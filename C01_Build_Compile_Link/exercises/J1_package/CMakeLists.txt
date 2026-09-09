cmake_minimum_required(VERSION 3.28)

project(J1_package VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

set(_lesson_api_dir "${CMAKE_CURRENT_LIST_DIR}/../E1_abi/include")
set(_lesson_api_src "${CMAKE_CURRENT_LIST_DIR}/../E1_abi/src/reference/lesson_api.cpp")

add_library(lesson_static STATIC "${_lesson_api_src}")
target_include_directories(lesson_static
    PUBLIC
        $<BUILD_INTERFACE:${_lesson_api_dir}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(lesson_static PUBLIC LESSON_STATIC)
engineering_configure_target(lesson_static)

add_library(lesson_shared SHARED "${_lesson_api_src}")
target_include_directories(lesson_shared
    PUBLIC
        $<BUILD_INTERFACE:${_lesson_api_dir}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(lesson_shared PRIVATE LESSON_BUILD_SHARED)
engineering_configure_target(lesson_shared)

install(TARGETS lesson_static lesson_shared
    EXPORT LessonPackageTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES "${_lesson_api_dir}/lesson_api.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT LessonPackageTargets
    NAMESPACE LessonPackage::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LessonPackage
)

configure_package_config_file(
    cmake/LessonPackageConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LessonPackageConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LessonPackage
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/LessonPackageConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LessonPackageConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/LessonPackageConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LessonPackage
)

if(ENGINEERING_STUDY_BUILD_REFERENCE)
    engineering_add_test(J1_package_roundtrip
        COMMAND
            ${CMAKE_COMMAND}
            -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -DCONFIG=$<CONFIG>
            -DGENERATOR=${CMAKE_GENERATOR}
            -DPLATFORM=${CMAKE_GENERATOR_PLATFORM}
            -DINSTANCE=${CMAKE_GENERATOR_INSTANCE}
            -DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_package_roundtrip.cmake
        LABELS reference
        TIMEOUT 180
    )
endif()
