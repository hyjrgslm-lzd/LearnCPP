cmake_minimum_required(VERSION 3.28)
project(F2_dependencies LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")

set(F2_DEPENDENCY_MODE "fetchcontent" CACHE STRING "fetchcontent or find_package")
set_property(CACHE F2_DEPENDENCY_MODE PROPERTY STRINGS fetchcontent find_package)

if(F2_DEPENDENCY_MODE STREQUAL "find_package")
    find_package(F2Provider 1.0 CONFIG REQUIRED)
elseif(F2_DEPENDENCY_MODE STREQUAL "fetchcontent")
    include(FetchContent)
    set(F2_PROVIDER_FIXTURE_SOURCE_DIR
        "${CMAKE_CURRENT_LIST_DIR}/provider_fixture"
        CACHE PATH "Offline source directory for the fixed F2 provider fixture"
    )
    if(NOT EXISTS "${F2_PROVIDER_FIXTURE_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "F2_PROVIDER_FIXTURE_SOURCE_DIR must point to the local F2 provider fixture source tree: ${F2_PROVIDER_FIXTURE_SOURCE_DIR}"
        )
    endif()
    FetchContent_Declare(f2_provider_fixture
        SOURCE_DIR "${F2_PROVIDER_FIXTURE_SOURCE_DIR}"
    )
    FetchContent_MakeAvailable(f2_provider_fixture)
else()
    message(FATAL_ERROR "F2_DEPENDENCY_MODE must be fetchcontent or find_package")
endif()

add_executable(F2_dependencies_reference checks/reference_check.cpp)
target_link_libraries(F2_dependencies_reference PRIVATE F2Provider::provider)
engineering_configure_target(F2_dependencies_reference)

add_executable(F2_dependencies_student checks/student_check.cpp src/student/student.cpp)
target_include_directories(F2_dependencies_student PRIVATE src/student)
target_link_libraries(F2_dependencies_student PRIVATE F2Provider::provider)
engineering_configure_target(F2_dependencies_student)

add_executable(F2_dependencies_student_delegation
    checks/delegation_check.cpp
    checks/spy_provider/spy_provider.cpp
    src/student/student.cpp
)
target_include_directories(F2_dependencies_student_delegation
    PRIVATE
        src/student
        checks/spy_provider
        checks/spy_provider/include
)
engineering_configure_target(F2_dependencies_student_delegation)

engineering_add_test(F2_dependencies_reference
    COMMAND F2_dependencies_reference
    LABELS reference
    TIMEOUT 30
)

if(ENGINEERING_STUDY_TEST_STUDENTS)
    engineering_add_test(F2_dependencies_student
        COMMAND F2_dependencies_student
        LABELS student
        TIMEOUT 30
    )
    engineering_add_test(F2_dependencies_student_delegation
        COMMAND F2_dependencies_student_delegation
        LABELS student
        TIMEOUT 30
    )
endif()
