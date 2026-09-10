cmake_minimum_required(VERSION 3.28)
project(F01_frontier LANGUAGES CXX)
include(../cmake/StudySetup.cmake)
include(CheckCXXSourceCompiles)

if(NOT GENERIC_STUDY_ENABLE_FRONTIER)
    message(STATUS "F01_frontier tests are OFF: set GENERIC_STUDY_ENABLE_FRONTIER=ON to register capability probes")
    return()
endif()

option(GENERIC_STUDY_FRONTIER_ENABLE_FAILURE_CONTROL
    "Register a controlled probe that proves declared-capability body failures are reported as FAIL"
    OFF)

function(f01_compile_flag out standard)
    if(MSVC)
        set(${out} "/std:c++latest" PARENT_SCOPE)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        if(standard STREQUAL "29")
            set(${out} "-std=c++2y" PARENT_SCOPE)
        else()
            set(${out} "-std=c++2c" PARENT_SCOPE)
        endif()
    else()
        set(${out} "" PARENT_SCOPE)
    endif()
endfunction()

f01_compile_flag(F01_CXX26_FLAG 26)
f01_compile_flag(F01_CXX29_FLAG 29)

function(f01_required_flags out standard)
    f01_compile_flag(flags ${standard})
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        string(APPEND flags " -freflection")
    endif()
    set(${out} "${flags}" PARENT_SCOPE)
endfunction()

f01_required_flags(F01_CXX26_REQUIRED_FLAGS 26)
f01_required_flags(F01_CXX29_REQUIRED_FLAGS 29)

set(CMAKE_REQUIRED_FLAGS "${F01_CXX26_REQUIRED_FLAGS}")
check_cxx_source_compiles("
#include <memory>
#include <new>
constexpr int construct() {
    std::allocator<int> allocator;
    int* p = allocator.allocate(1);
    ::new (p) int(7);
    int value = *p;
    std::destroy_at(p);
    allocator.deallocate(p, 1);
    return value;
}
static_assert(construct() == 7);
int main() {}
" F01_HAS_CONSTEXPR_PLACEMENT_NEW)

check_cxx_source_compiles("
#include <meta>
struct [[=1]] AnnotatedType {};
int main() {}
" F01_HAS_ANNOTATIONS)

check_cxx_source_compiles("
template<class T>
concept Basic = requires { typename T::value_type; };
template<class T>
concept Refined = Basic<T> && true;
template<class... Ts>
    requires (Basic<Ts> && ...)
constexpr int rank(Ts...) { return 1; }
template<class... Ts>
    requires (Refined<Ts> && ...)
constexpr int rank(Ts...) { return 2; }
struct WithValueType { using value_type = int; };
static_assert(rank(WithValueType{}, WithValueType{}) == 2);
int main() {}
" F01_HAS_FOLD_EXPANDED_CONSTRAINTS)

set(CMAKE_REQUIRED_FLAGS "${F01_CXX29_REQUIRED_FLAGS}")
check_cxx_source_compiles("
#include <concepts>
struct Throws { int operator()(); };
struct NoThrow { int operator()() noexcept; };
template<class F, bool RequiredNoexcept>
concept ConditionallyCallable = requires(F f) {
    { f() } noexcept(RequiredNoexcept) -> std::same_as<int>;
};
static_assert(ConditionallyCallable<Throws, false>);
static_assert(!ConditionallyCallable<Throws, true>);
static_assert(ConditionallyCallable<NoThrow, true>);
int main() {}
" F01_HAS_CONDITIONAL_NOEXCEPT_REQUIREMENT)

check_cxx_source_compiles("
#include <type_traits>
template<class T> struct Box { using type = T; };
template<class T> struct Wrap { using type = T*; };
template<template<class> class... TT>
struct PickTemplate {
    template<class T> using first = TT...[0]<T>;
    template<class T> using second = TT...[1]<T>;
};
using Pick = PickTemplate<Box, Wrap>;
static_assert(std::is_same_v<typename Pick::first<int>::type, int>);
static_assert(std::is_same_v<typename Pick::second<int>::type, int*>);
int main() {}
" F01_HAS_TEMPLATE_NAME_PACK_INDEXING)

unset(CMAKE_REQUIRED_FLAGS)
message(STATUS "F01 required flags C++26: ${F01_CXX26_REQUIRED_FLAGS}")
message(STATUS "F01 required flags C++29: ${F01_CXX29_REQUIRED_FLAGS}")
message(STATUS "F01 constexpr placement new behavior: ${F01_HAS_CONSTEXPR_PLACEMENT_NEW}")
message(STATUS "F01 minimal annotations syntax: ${F01_HAS_ANNOTATIONS}")
message(STATUS "F01 fold expanded constraints behavior: ${F01_HAS_FOLD_EXPANDED_CONSTRAINTS}")
message(STATUS "F01 C++29 conditional noexcept compound requirement behavior: ${F01_HAS_CONDITIONAL_NOEXCEPT_REQUIREMENT}")
message(STATUS "F01 C++29 template-name pack indexing behavior: ${F01_HAS_TEMPLATE_NAME_PACK_INDEXING}")

function(f01_configure_frontier target standard)
    c04_configure_target(${target})
    if(MSVC)
        target_compile_options(${target} PRIVATE /std:c++latest)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        f01_compile_flag(flag ${standard})
        target_compile_options(${target} PRIVATE ${flag})
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE -freflection)
        endif()
    endif()
endfunction()

function(f01_add_capability name source)
    cmake_parse_arguments(ARG "" "STANDARD" "" ${ARGN})
    if(NOT ARG_STANDARD)
        set(ARG_STANDARD 26)
    endif()
    add_executable(${name} ${source})
    f01_configure_frontier(${name} ${ARG_STANDARD})
    c04_add_test(${name} LABELS capability TIMEOUT 30)
    if(BUILD_TESTING)
        set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77)
    endif()
endfunction()

f01_add_capability(F01_reflection_query probes/reflection_query_probe.cpp)
f01_add_capability(F01_splicing_generation probes/splicing_generation_probe.cpp)
f01_add_capability(F01_annotations probes/annotations_probe.cpp)
if(F01_HAS_ANNOTATIONS)
    target_compile_definitions(F01_annotations PRIVATE C04_HAS_ANNOTATION_SYNTAX=1)
endif()
f01_add_capability(F01_expansion_statement probes/expansion_statement_probe.cpp)
f01_add_capability(F01_define_static probes/define_static_probe.cpp)
f01_add_capability(F01_define_aggregate probes/define_aggregate_probe.cpp)
f01_add_capability(F01_pack_indexing probes/pack_indexing_probe.cpp)
f01_add_capability(F01_fold_constraints probes/fold_constraints_probe.cpp)
if(F01_HAS_FOLD_EXPANDED_CONSTRAINTS)
    target_compile_definitions(F01_fold_constraints PRIVATE C04_HAS_FOLD_EXPANDED_CONSTRAINTS=1)
endif()
f01_add_capability(F01_constexpr_placement_new probes/constexpr_placement_new_probe.cpp)
if(F01_HAS_CONSTEXPR_PLACEMENT_NEW)
    target_compile_definitions(F01_constexpr_placement_new PRIVATE C04_HAS_CONSTEXPR_PLACEMENT_NEW=1)
endif()
f01_add_capability(F01_constexpr_exceptions probes/constexpr_exceptions_probe.cpp)
f01_add_capability(F01_c29_conditional_noexcept_requirement probes/c29_conditional_noexcept_requirement_probe.cpp STANDARD 29)
if(F01_HAS_CONDITIONAL_NOEXCEPT_REQUIREMENT)
    target_compile_definitions(F01_c29_conditional_noexcept_requirement PRIVATE C04_HAS_CONDITIONAL_NOEXCEPT_REQUIREMENT=1)
endif()
f01_add_capability(F01_c29_template_name_pack_indexing probes/c29_template_name_pack_indexing_probe.cpp STANDARD 29)
if(F01_HAS_TEMPLATE_NAME_PACK_INDEXING)
    target_compile_definitions(F01_c29_template_name_pack_indexing PRIVATE C04_HAS_TEMPLATE_NAME_PACK_INDEXING=1)
endif()
f01_add_capability(F01_consteval_only_values probes/consteval_only_values_probe.cpp)
f01_add_capability(F01_attributes_reflection_proposal probes/attributes_reflection_proposal_probe.cpp)
if(GENERIC_STUDY_FRONTIER_ENABLE_FAILURE_CONTROL)
    f01_add_capability(F01_declared_failure_control probes/declared_failure_control_probe.cpp)
endif()
