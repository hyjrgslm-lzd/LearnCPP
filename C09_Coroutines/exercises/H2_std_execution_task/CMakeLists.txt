cmake_minimum_required(VERSION 3.25)
project(H2_std_execution_task LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /Zc:preprocessor)
endif()

include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

set(H2_CXX26_PREVIEW_FLAG "")
set(H2_HAS_CXX26_PREVIEW_FLAG OFF)
if(MSVC)
    set(H2_CXX26_PREVIEW_FLAG "/std:c++latest")
    set(H2_HAS_CXX26_PREVIEW_FLAG ON)
else()
    check_cxx_compiler_flag("-std=c++2c" H2_HAS_STDCXX2C_FLAG)
    if(H2_HAS_STDCXX2C_FLAG)
        set(H2_CXX26_PREVIEW_FLAG "-std=c++2c")
        set(H2_HAS_CXX26_PREVIEW_FLAG ON)
    else()
        check_cxx_compiler_flag("-std=c++26" H2_HAS_STDCXX26_FLAG)
        if(H2_HAS_STDCXX26_FLAG)
            set(H2_CXX26_PREVIEW_FLAG "-std=c++26")
            set(H2_HAS_CXX26_PREVIEW_FLAG ON)
        endif()
    endif()
endif()

set(CMAKE_REQUIRED_FLAGS_SAVED "${CMAKE_REQUIRED_FLAGS}")
if(H2_CXX26_PREVIEW_FLAG)
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${H2_CXX26_PREVIEW_FLAG}")
endif()
check_cxx_source_compiles([=[
    #include <execution>
    #include <thread>
    #include <tuple>

    std::execution::task<int> one()
    {
        co_return 1;
    }

    int main()
    {
        auto result = std::this_thread::sync_wait(one());
        return result && std::get<0>(*result) == 1 ? 0 : 1;
    }
]=] H2_HAS_STD_EXECUTION_TASK)
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_SAVED}")

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
    add_executable(H2_std_execution_task checks/main.cpp)
    target_compile_features(H2_std_execution_task PRIVATE cxx_std_23)
    target_include_directories(H2_std_execution_task PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${COROUTINE_STUDY_EXERCISES_DIR}/include")
    target_link_libraries(H2_std_execution_task PRIVATE stdexec::stdexec)
    if(BUILD_TESTING AND COROUTINE_STUDY_TEST_STARTERS)
        add_test(NAME H2_std_execution_task COMMAND H2_std_execution_task)
        coroutine_study_set_test_defaults(H2_std_execution_task starter 30)
    endif()
    coroutine_study_add_checker_variants(H2_std_execution_task)
else()
    add_executable(H2_std_execution_task checks/main.cpp)
    target_include_directories(H2_std_execution_task PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(H2_std_execution_task PRIVATE stdexec::stdexec)
endif()

add_executable(H2_std_task_probe std_task_probe.cpp)
target_compile_features(H2_std_task_probe PRIVATE cxx_std_23)
target_include_directories(H2_std_task_probe PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
if(H2_CXX26_PREVIEW_FLAG)
    target_compile_options(H2_std_task_probe PRIVATE "${H2_CXX26_PREVIEW_FLAG}")
endif()
if(BUILD_TESTING)
    add_test(NAME H2_std_task_probe COMMAND H2_std_task_probe)
    set_tests_properties(H2_std_task_probe PROPERTIES
        LABELS "probe"
        TIMEOUT 30
        SKIP_RETURN_CODE 77
    )
endif()

if(COROUTINE_STUDY_BUILD_REFERENCE AND H2_STDEXEC_TASK_REFERENCE_SUPPORTED)
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
        "It requires COROUTINE_STUDY_BUILD_REFERENCE=ON and a supported stdexec task lane "
        "(MSVC, or GNU on Linux).")
endif()
