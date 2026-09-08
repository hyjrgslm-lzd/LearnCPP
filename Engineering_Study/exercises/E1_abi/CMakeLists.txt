cmake_minimum_required(VERSION 3.28)

project(E1_abi LANGUAGES C CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

function(e1_configure_lesson_library target)
    target_include_directories(${target} PUBLIC include)
    engineering_configure_target(${target})
endfunction()

add_library(E1_lesson_reference_static STATIC src/reference/lesson_api.cpp)
target_compile_definitions(E1_lesson_reference_static PUBLIC LESSON_STATIC)
e1_configure_lesson_library(E1_lesson_reference_static)

add_library(E1_lesson_reference_shared SHARED src/reference/lesson_api.cpp)
target_compile_definitions(E1_lesson_reference_shared PRIVATE LESSON_BUILD_SHARED)
e1_configure_lesson_library(E1_lesson_reference_shared)

add_library(E1_lesson_bad_alloc_static STATIC src/reference/lesson_api.cpp)
target_compile_definitions(E1_lesson_bad_alloc_static PUBLIC LESSON_STATIC PRIVATE LESSON_FORCE_BAD_ALLOC)
e1_configure_lesson_library(E1_lesson_bad_alloc_static)

add_executable(E1_abi_reference_static checks/abi_contract_check.cpp)
target_link_libraries(E1_abi_reference_static PRIVATE E1_lesson_reference_static)
engineering_configure_target(E1_abi_reference_static)

add_executable(E1_abi_reference_shared checks/abi_contract_check.cpp)
target_link_libraries(E1_abi_reference_shared PRIVATE E1_lesson_reference_shared)
engineering_configure_target(E1_abi_reference_shared)

add_executable(E1_abi_bad_alloc checks/bad_alloc_check.cpp)
target_link_libraries(E1_abi_bad_alloc PRIVATE E1_lesson_bad_alloc_static)
engineering_configure_target(E1_abi_bad_alloc)

add_executable(E1_abi_c_consumer c_consumer/main.c)
target_link_libraries(E1_abi_c_consumer PRIVATE E1_lesson_reference_shared)

add_executable(E1_abi_observation_layout observation/layout.cpp)
engineering_configure_target(E1_abi_observation_layout)

if(ENGINEERING_STUDY_TEST_STUDENTS)
    add_library(E1_lesson_student_static STATIC src/student/lesson_api.cpp)
    target_compile_definitions(E1_lesson_student_static PUBLIC LESSON_STATIC)
    e1_configure_lesson_library(E1_lesson_student_static)

    add_executable(E1_abi_student checks/abi_contract_check.cpp)
    target_link_libraries(E1_abi_student PRIVATE E1_lesson_student_static)
    engineering_configure_target(E1_abi_student)

    engineering_add_test(E1_abi_student
        COMMAND E1_abi_student student
        LABELS student
        TIMEOUT 30
    )
endif()

if(ENGINEERING_STUDY_BUILD_REFERENCE)
    engineering_add_test(E1_abi_reference_static
        COMMAND E1_abi_reference_static static
        LABELS reference
        TIMEOUT 30
    )
    engineering_add_test(E1_abi_reference_shared
        COMMAND E1_abi_reference_shared shared
        LABELS reference
        TIMEOUT 30
    )
    engineering_add_test(E1_abi_bad_alloc
        COMMAND E1_abi_bad_alloc
        LABELS reference
        TIMEOUT 30
    )
    engineering_add_test(E1_abi_c_consumer
        COMMAND E1_abi_c_consumer
        LABELS reference
        TIMEOUT 30
    )
endif()

engineering_add_test(E1_abi_observation_layout
    COMMAND E1_abi_observation_layout
    LABELS observation
    TIMEOUT 30
)
