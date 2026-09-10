cmake_minimum_required(VERSION 3.28)
project(advanced_review_r2_h3_probe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_CXX26_STANDARD_COMPILE_OPTION "/std:c++latest")
    set(CMAKE_CXX26_EXTENSION_COMPILE_OPTION "/std:c++latest")
endif()

set(PROBE_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../advanced-review-r2-h3-hostile-probe.cpp")
set(H3_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../exercises/H3_generator_const_iter")

foreach(kind IN ITEMS reference good bad student)
    add_executable(h3_probe_${kind} "${PROBE_SOURCE}")
endforeach()

target_include_directories(h3_probe_reference PRIVATE "${H3_ROOT}/src/reference")
target_include_directories(h3_probe_good PRIVATE "${H3_ROOT}/validation/good")
target_include_directories(h3_probe_bad PRIVATE "${H3_ROOT}/validation/bad")
target_include_directories(h3_probe_student PRIVATE "${H3_ROOT}/src/student")
