include_guard(GLOBAL)
get_filename_component(CONCURRENCY_STUDY_EXERCISES_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/ExerciseIde.cmake" OPTIONAL)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# This course uses ordinary translation units, not module interface units.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(CONCURRENCY_STUDY_BUILD_REFERENCE "Build checked reference implementations" ON)
option(CONCURRENCY_STUDY_TEST_STARTERS "Register implementation tests and observation smoke checks separately" OFF)
option(CONCURRENCY_STUDY_TEST_FAST_MATH "Build the isolated strict-checker/fast-kernel experiment" OFF)
option(CONCURRENCY_STUDY_FETCH_DEPS "Allow fetching pinned optional dependencies" OFF)
option(CONCURRENCY_STUDY_ENABLE_XSIMD "Enable pinned xsimd examples" OFF)
option(CONCURRENCY_STUDY_ENABLE_STDEXEC "Enable pinned stdexec examples" OFF)
option(CONCURRENCY_STUDY_ENABLE_CXX26 "Probe native C++26 library examples" OFF)
option(CONCURRENCY_STUDY_ENABLE_CXX29 "Probe N5054 thread attributes and hazard pointer batches" OFF)
option(CONCURRENCY_STUDY_BUILD_BENCHMARKS "Build standalone measurement programs" OFF)
option(CONCURRENCY_STUDY_ENABLE_UNSAFE_DEMOS "Explicitly enable isolated unsafe examples" OFF)
include("${CMAKE_CURRENT_LIST_DIR}/Sanitizers.cmake")
if(CONCURRENCY_STUDY_SANITIZER STREQUAL "none")
    set(CS_TEST_TIMEOUT 30)
else()
    set(CS_TEST_TIMEOUT 120)
endif()

include(CTest)
find_package(Threads REQUIRED)
include(CheckCXXSourceCompiles)
find_package(TBB QUIET CONFIG)
set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
if(TARGET TBB::tbb)
    list(APPEND CMAKE_REQUIRED_LIBRARIES TBB::tbb)
endif()
unset(CS_HAS_PARALLEL_ALGORITHMS CACHE)
check_cxx_source_compiles("#include <algorithm>
#include <execution>
#include <numeric>
#include <vector>
int main() { std::vector<int> v{3,1,2};
std::sort(std::execution::par, v.begin(), v.end());
return std::reduce(std::execution::par,v.begin(),v.end(),0) != 6; }"
    CS_HAS_PARALLEL_ALGORITHMS)
unset(CMAKE_REQUIRED_LIBRARIES)

set(CS_HAS_XSIMD 0)
set(CS_HAS_STDEXEC 0)
if(CONCURRENCY_STUDY_ENABLE_XSIMD)
    include("${CMAKE_CURRENT_LIST_DIR}/XsimdSetup.cmake")
    set(CS_HAS_XSIMD 1)
endif()
if(CONCURRENCY_STUDY_ENABLE_STDEXEC)
    include("${CMAKE_CURRENT_LIST_DIR}/StdexecSetup.cmake")
    set(CS_HAS_STDEXEC 1)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/NativeFeatures.cmake")

function(cs_add_visible_files target)
    set(_cs_files ${ARGN})
    foreach(_cs_file IN ITEMS
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/BUILD_GUIDE.md"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/CMakePresets.json"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/third_party/README.md"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/benchmark.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/bounded_channel.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/epoch.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/exercise_check.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/hazard_pointer.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/log.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/numa.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/numeric_kernels.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/queue_baseline.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/queue_checks.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/queue_linked.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/queue_versions.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/rcu.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/simd_kernels.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/thread_pool.hpp"
        "${CONCURRENCY_STUDY_EXERCISES_DIR}/include/concurrency_study/work_stealing_pool.hpp")
        if(EXISTS "${_cs_file}")
            list(APPEND _cs_files "${_cs_file}")
        endif()
    endforeach()
    foreach(_cs_file IN LISTS _cs_files)
        if(EXISTS "${_cs_file}")
            list(APPEND _cs_existing "${_cs_file}")
        endif()
    endforeach()
    if(_cs_existing)
        target_sources(${target} PRIVATE ${_cs_existing})
    endif()
endfunction()

function(cs_exercise_local_files name role out_var)
    set(_cs_files "${CMAKE_CURRENT_SOURCE_DIR}/checks.hpp")
    if(name MATCHES "^(I3_rcu|R1_epoch_reclamation|R2_qsbr)$")
        list(APPEND _cs_files "${CONCURRENCY_STUDY_EXERCISES_DIR}/../topics/reclamation/experiment_support.hpp")
    endif()
    if(role STREQUAL "MAIN")
        list(APPEND _cs_files "${CMAKE_CURRENT_SOURCE_DIR}/student.hpp")
        if(name MATCHES "^(G2_aba_problem|J1_false_sharing|J2_interference_size)$")
            list(APPEND _cs_files "${CMAKE_CURRENT_SOURCE_DIR}/reference.hpp")
        endif()
        if(name STREQUAL "J2_interference_size")
            list(APPEND _cs_files "${CONCURRENCY_STUDY_EXERCISES_DIR}/J1_false_sharing/reference.hpp")
        endif()
    elseif(role STREQUAL "REFERENCE")
        list(APPEND _cs_files "${CMAKE_CURRENT_SOURCE_DIR}/reference.hpp")
        if(name STREQUAL "J2_interference_size")
            list(APPEND _cs_files "${CONCURRENCY_STUDY_EXERCISES_DIR}/J1_false_sharing/reference.hpp")
        endif()
    elseif(role STREQUAL "BENCHMARK")
        list(APPEND _cs_files "${CMAKE_CURRENT_SOURCE_DIR}/reference.hpp")
    endif()
    set(${out_var} ${_cs_files} PARENT_SCOPE)
endfunction()

function(cs_configure_target target)
    target_compile_features(${target} PRIVATE cxx_std_23)
    cs_enable_sanitizer(${target})
    target_include_directories(${target} PRIVATE "${CONCURRENCY_STUDY_EXERCISES_DIR}/include")
    target_link_libraries(${target} PRIVATE Threads::Threads)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # libstdc++'s runtime lock-free query needs this compiler runtime on Linux.
        target_link_libraries(${target} PRIVATE atomic)
    endif()
    target_compile_definitions(${target} PRIVATE
        CS_HAS_XSIMD=${CS_HAS_XSIMD}
        CS_HAS_STDEXEC=${CS_HAS_STDEXEC}
        CS_HAS_STD_SIMD=${CS_HAS_STD_SIMD}
        CS_HAS_STD_SENDERS=${CS_HAS_STD_SENDERS}
        CS_HAS_STD_HAZARD_POINTER=${CS_HAS_STD_HAZARD_POINTER}
        CS_HAS_STD_RCU=${CS_HAS_STD_RCU}
        CS_HAS_STD_THREAD_ATTRIBUTES=${CS_HAS_STD_THREAD_ATTRIBUTES}
        CS_HAS_STD_HAZARD_POINTER_BATCH=${CS_HAS_STD_HAZARD_POINTER_BATCH}
        CS_HAS_ATOMIC_MIN_MAX=${CS_HAS_ATOMIC_MIN_MAX}
        CS_HAS_INPLACE_STOP_TOKEN=${CS_HAS_INPLACE_STOP_TOKEN}
        CS_HAS_PARALLEL_ALGORITHMS=$<BOOL:${CS_HAS_PARALLEL_ALGORITHMS}>
        CS_ENABLE_UNSAFE_DEMOS=$<BOOL:${CONCURRENCY_STUDY_ENABLE_UNSAFE_DEMOS}>)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /EHsc /Zc:__cplusplus /Zc:preprocessor /W4 /FS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(CONCURRENCY_STUDY_ENABLE_CXX26 OR CONCURRENCY_STUDY_ENABLE_CXX29)
        # Per-source options follow the ordinary C++23 target flag, including
        # MSVC versions for which CMake has not yet exposed cxx_std_26.
        get_target_property(_cs_sources ${target} SOURCES)
        if(MSVC)
            set_source_files_properties(${_cs_sources} PROPERTIES COMPILE_OPTIONS /std:c++latest)
        else()
            set_source_files_properties(${_cs_sources} PROPERTIES COMPILE_OPTIONS -std=c++2c)
        endif()
    endif()
    if(CS_HAS_PARALLEL_ALGORITHMS AND TARGET TBB::tbb)
        target_link_libraries(${target} PRIVATE TBB::tbb)
    endif()
    if(CS_HAS_XSIMD)
        target_link_libraries(${target} PRIVATE xsimd)
    endif()
    if(CS_HAS_STDEXEC)
        target_link_libraries(${target} PRIVATE stdexec::stdexec)
    endif()
endfunction()

function(cs_add_exercise name kind)
    if(NOT kind MATCHES "^(IMPLEMENTATION|OBSERVATION)$")
        message(FATAL_ERROR "${name}: explicitly classify main as IMPLEMENTATION or OBSERVATION")
    endif()
    add_executable(${name} main.cpp)
    cs_configure_target(${name})
    cs_exercise_local_files(${name} MAIN _cs_main_files)
    cs_add_visible_files(${name} ${_cs_main_files})
    if(BUILD_TESTING AND CONCURRENCY_STUDY_TEST_STARTERS)
        if(kind STREQUAL "IMPLEMENTATION")
            set(_cs_role student)
        else()
            set(_cs_role observation)
        endif()
        add_test(NAME ${name}_${_cs_role} COMMAND ${name})
        set_tests_properties(${name}_${_cs_role} PROPERTIES TIMEOUT ${CS_TEST_TIMEOUT} LABELS "${_cs_role}" SKIP_RETURN_CODE 77)
    endif()
    if(WIN32 AND name MATCHES "^(J3_numa_concept|N1_numa_placement)$")
        target_link_libraries(${name} PRIVATE Psapi)
    endif()
    file(GLOB _cs_reference CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/solution.cpp")
    file(GLOB _cs_benchmark CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark.cpp")
    if(CONCURRENCY_STUDY_BUILD_BENCHMARKS AND _cs_benchmark)
        add_executable(${name}_benchmark benchmark.cpp)
        cs_configure_target(${name}_benchmark)
        cs_exercise_local_files(${name} BENCHMARK _cs_benchmark_files)
        cs_add_visible_files(${name}_benchmark ${_cs_benchmark_files})
    endif()
    if(CONCURRENCY_STUDY_BUILD_REFERENCE AND _cs_reference)
        add_executable(${name}_reference solution.cpp)
        cs_configure_target(${name}_reference)
        cs_exercise_local_files(${name} REFERENCE _cs_reference_files)
        cs_add_visible_files(${name}_reference ${_cs_reference_files})
        if(WIN32 AND name MATCHES "^(J3_numa_concept|N1_numa_placement)$")
            target_link_libraries(${name}_reference PRIVATE Psapi)
        endif()
        if(BUILD_TESTING)
            add_test(NAME ${name}_reference COMMAND ${name}_reference)
            set_tests_properties(${name}_reference PROPERTIES TIMEOUT ${CS_TEST_TIMEOUT} SKIP_RETURN_CODE 77 LABELS "reference")
        endif()
    endif()
    if(COMMAND learncpp_setup_ide)
        learncpp_setup_ide(${name})
    endif()
endfunction()
