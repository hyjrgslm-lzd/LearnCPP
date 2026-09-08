cmake_minimum_required(VERSION 4.2)
project(H1_modules LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

add_executable(H1_header_baseline header_baseline/main.cpp)
engineering_configure_target(H1_header_baseline)

add_executable(H1_modules_reference)
target_sources(H1_modules_reference
    PRIVATE
        reference/main.cpp
        reference/geometry.cpp
    PRIVATE
        FILE_SET CXX_MODULES
        FILES
            reference/geometry.ixx
            reference/geometry.partition.ixx
)
engineering_configure_target(H1_modules_reference)
set_property(TARGET H1_modules_reference PROPERTY CXX_SCAN_FOR_MODULES ON)


add_executable(H1_visibility_boundary)
target_sources(H1_visibility_boundary
    PRIVATE
        reference/visibility_main.cpp
    PRIVATE
        FILE_SET CXX_MODULES
        FILES
            reference/visibility.ixx
)
engineering_configure_target(H1_visibility_boundary)
set_property(TARGET H1_visibility_boundary PROPERTY CXX_SCAN_FOR_MODULES ON)

add_executable(H1_global_private_fragment)
target_sources(H1_global_private_fragment
    PRIVATE
        reference/fragments_main.cpp
    PRIVATE
        FILE_SET CXX_MODULES
        FILES
            reference/fragments.ixx
)
engineering_configure_target(H1_global_private_fragment)
set_property(TARGET H1_global_private_fragment PROPERTY CXX_SCAN_FOR_MODULES ON)

add_executable(H1_hidden_type_negative EXCLUDE_FROM_ALL)
target_sources(H1_hidden_type_negative
    PRIVATE
        negative/direct_name_hidden.cpp
    PRIVATE
        FILE_SET CXX_MODULES
        FILES
            reference/visibility.ixx
)
engineering_configure_target(H1_hidden_type_negative)
set_property(TARGET H1_hidden_type_negative PROPERTY CXX_SCAN_FOR_MODULES ON)

engineering_add_test(H1_header_baseline
    COMMAND H1_header_baseline
    LABELS reference
    TIMEOUT 30
)
engineering_add_test(H1_modules_reference
    COMMAND H1_modules_reference
    LABELS reference
    TIMEOUT 30
)
engineering_add_test(H1_visibility_boundary
    COMMAND H1_visibility_boundary
    LABELS reference
    TIMEOUT 30
)
engineering_add_test(H1_global_private_fragment
    COMMAND H1_global_private_fragment
    LABELS reference
    TIMEOUT 30
)
engineering_add_test(H1_modules_package_consumer
    COMMAND
        ${CMAKE_COMMAND}
        -DCASE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/package/library
        -DCONSUMER_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/package/consumer
        -DCASE_BUILD=${CMAKE_CURRENT_BINARY_DIR}/package/library-build
        -DINSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/package/prefix
        -DCONSUMER_BUILD=${CMAKE_CURRENT_BINARY_DIR}/package/consumer-build
        -DGENERATOR=${CMAKE_GENERATOR}
        -DCONFIG=$<CONFIG>
        -DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_package_consumer.cmake
    LABELS reference
    TIMEOUT 180
)
