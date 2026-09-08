cmake_minimum_required(VERSION 3.28)

project(D1_shared_library LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

add_library(D1_runtime_static STATIC src/runtime.cpp)
target_include_directories(D1_runtime_static PUBLIC include)
engineering_configure_target(D1_runtime_static)

add_library(D1_runtime_shared SHARED src/runtime.cpp)
target_include_directories(D1_runtime_shared PUBLIC include)
target_compile_definitions(D1_runtime_shared PRIVATE D1_RUNTIME_BUILD_SHARED)
engineering_configure_target(D1_runtime_shared)

add_executable(D1_static_reference checks/linked_check.cpp)
target_link_libraries(D1_static_reference PRIVATE D1_runtime_static)
engineering_configure_target(D1_static_reference)

add_executable(D1_shared_reference checks/linked_check.cpp)
target_link_libraries(D1_shared_reference PRIVATE D1_runtime_shared)
target_compile_definitions(D1_shared_reference PRIVATE D1_RUNTIME_USE_SHARED)
engineering_configure_target(D1_shared_reference)

add_executable(D1_loader_reference checks/loader_check.cpp)
engineering_configure_target(D1_loader_reference)
if(UNIX)
    target_link_libraries(D1_loader_reference PRIVATE dl)
endif()

engineering_add_test(D1_static_reference
    COMMAND D1_static_reference
    LABELS reference
    TIMEOUT 30
)
engineering_add_test(D1_shared_reference
    COMMAND D1_shared_reference
    LABELS reference
    TIMEOUT 30
)
engineering_add_test(D1_loader_reference
    COMMAND D1_loader_reference "$<TARGET_FILE:D1_runtime_shared>" lesson_runtime_value "${CMAKE_CURRENT_BINARY_DIR}/loader-exit.txt"
    LABELS reference
    TIMEOUT 30
)
engineering_add_test(D1_negative_missing_dll
    COMMAND
        ${CMAKE_COMMAND}
        -DEXE=$<TARGET_FILE:D1_shared_reference>
        -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/negative/missing_dll
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_missing_dll.cmake
    LABELS negative
    TIMEOUT 30
)
engineering_add_test(D1_negative_missing_export
    COMMAND
        ${CMAKE_COMMAND}
        -DLOADER=$<TARGET_FILE:D1_loader_reference>
        -DLIBRARY=$<TARGET_FILE:D1_runtime_shared>
        -DSYMBOL=lesson_missing_export
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_loader_failure.cmake
    LABELS negative
    TIMEOUT 30
)
