cmake_minimum_required(VERSION 3.25)
project(H2_std_execution_task LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /Zc:preprocessor)
endif()

include(CheckCXXSourceCompiles)

check_cxx_source_compiles([=[
    #include <execution>
    int main()
    {
        std::execution::task<void>* task = nullptr;
        (void)task;
        return 0;
    }
]=] H2_HAS_STD_EXECUTION_TASK)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/std_task_probe_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/std_task_probe_config.hpp"
)

set(H2_STDEXEC_TASK_REFERENCE_SUPPORTED OFF)
if(MSVC)
    set(H2_STDEXEC_TASK_REFERENCE_SUPPORTED ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(H2_STDEXEC_TASK_REFERENCE_SUPPORTED ON)
endif()

if(COMMAND coroutine_study_add_exercise)
    coroutine_study_add_exercise(
        H2_std_execution_task
        SOURCES main.cpp
        LIBRARIES stdexec::stdexec
    )
else()
    add_executable(H2_std_execution_task main.cpp)
    target_link_libraries(H2_std_execution_task PRIVATE stdexec::stdexec)
endif()

add_executable(H2_std_task_probe std_task_probe.cpp)
target_compile_features(H2_std_task_probe PRIVATE cxx_std_23)
target_include_directories(H2_std_task_probe PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

if(H2_STDEXEC_TASK_REFERENCE_SUPPORTED)
    add_executable(H2_std_execution_task_reference solution.cpp)
    target_compile_features(H2_std_execution_task_reference PRIVATE cxx_std_23)
    if(DEFINED COROUTINE_STUDY_EXERCISES_DIR)
        target_include_directories(H2_std_execution_task_reference PRIVATE "${COROUTINE_STUDY_EXERCISES_DIR}/include")
    endif()
    target_link_libraries(H2_std_execution_task_reference PRIVATE stdexec::stdexec)
    if(BUILD_TESTING)
        add_test(NAME H2_std_execution_task_reference COMMAND H2_std_execution_task_reference)
        set_tests_properties(H2_std_execution_task_reference PROPERTIES
            LABELS "reference;stdexec"
            TIMEOUT 30
        )
    endif()
else()
    message(STATUS
        "H2_std_execution_task_reference is not built on this toolchain. "
        "Supported reference lanes: MSVC, or GNU on Linux. "
        "This avoids treating an unsupported toolchain as a passing CTest.")
endif()
