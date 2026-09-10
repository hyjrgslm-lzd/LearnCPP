cmake_minimum_required(VERSION 3.28)
project(C08_Concurrency LANGUAGES CXX)
include(cmake/StudySetup.cmake)

# One source of registration: each actual exercise has main.cpp and a leaf
# CMakeLists.txt. Build directories and auxiliary directories are excluded.
file(GLOB exercise_entries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*")
foreach(entry IN LISTS exercise_entries)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${entry}"
       AND NOT entry MATCHES "^build"
       AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${entry}/main.cpp"
       AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${entry}/CMakeLists.txt")
        add_subdirectory("${entry}")
    endif()
endforeach()
# A collection of independent native subjects, without a synthetic summary test.
add_subdirectory(F03_native_facilities)
add_subdirectory(runtime_tests)
if(CONCURRENCY_STUDY_BUILD_BENCHMARKS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(benchmarks)
endif()
