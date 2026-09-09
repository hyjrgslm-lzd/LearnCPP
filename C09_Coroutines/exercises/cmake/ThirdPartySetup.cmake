include_guard(GLOBAL)

include(FetchContent)

function(_coroutine_study_bridge_target public_name real_name)
    if(TARGET ${public_name})
        return()
    endif()
    if(TARGET ${real_name})
        string(REPLACE "::" "_" bridge_name "coroutine_study_bridge_${public_name}")
        add_library(${bridge_name} INTERFACE)
        target_link_libraries(${bridge_name} INTERFACE ${real_name})
        add_library(${public_name} ALIAS ${bridge_name})
    endif()
endfunction()

function(_coroutine_study_require target feature)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "${feature} is enabled but ${target} is unavailable. Install it, set CMAKE_PREFIX_PATH/pkg-config paths, set FETCHCONTENT_SOURCE_DIR_*, or enable COROUTINE_STUDY_FETCH_DEPS for light deps.")
    endif()
endfunction()

function(coroutine_study_setup_stdexec)
    cmake_parse_arguments(ARG "REQUIRED" "" "" ${ARGN})
    if(TARGET stdexec::stdexec)
        return()
    endif()

    find_package(stdexec QUIET CONFIG)
    _coroutine_study_bridge_target(stdexec::stdexec STDEXEC::stdexec)

    if(NOT TARGET stdexec::stdexec AND COROUTINE_STUDY_FETCH_DEPS)
        FetchContent_Declare(
            stdexec
            GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
            GIT_TAG        nvhpc-26.05
            GIT_SHALLOW    TRUE
            GIT_PROGRESS   TRUE
        )
        set(STDEXEC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(STDEXEC_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(STDEXEC_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(stdexec)
        _coroutine_study_bridge_target(stdexec::stdexec STDEXEC::stdexec)
        if(NOT TARGET stdexec::stdexec AND TARGET stdexec)
            add_library(stdexec::stdexec ALIAS stdexec)
        endif()
    endif()

    if(ARG_REQUIRED)
        _coroutine_study_require(stdexec::stdexec "COROUTINE_STUDY_ENABLE_STDEXEC")
    endif()
endfunction()

function(coroutine_study_setup_asio)
    cmake_parse_arguments(ARG "REQUIRED" "" "" ${ARGN})
    if(TARGET asio::asio)
        return()
    endif()

    find_path(ASIO_INCLUDE_DIR asio.hpp)
    if(ASIO_INCLUDE_DIR)
        add_library(asio_standalone INTERFACE)
        target_include_directories(asio_standalone INTERFACE "${ASIO_INCLUDE_DIR}")
        target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
        add_library(asio::asio ALIAS asio_standalone)
    elseif(COROUTINE_STUDY_FETCH_DEPS)
        FetchContent_Declare(
            asio
            GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
            GIT_TAG        asio-1-38-2
            GIT_SHALLOW    TRUE
            GIT_PROGRESS   TRUE
        )
        FetchContent_MakeAvailable(asio)
        if(EXISTS "${asio_SOURCE_DIR}/asio/include/asio.hpp")
            set(_asio_include_dir "${asio_SOURCE_DIR}/asio/include")
        elseif(EXISTS "${asio_SOURCE_DIR}/include/asio.hpp")
            set(_asio_include_dir "${asio_SOURCE_DIR}/include")
        else()
            message(FATAL_ERROR "COROUTINE_STUDY_ENABLE_ASIO is enabled but asio.hpp was not found in ${asio_SOURCE_DIR}.")
        endif()
        add_library(asio_standalone INTERFACE)
        target_include_directories(asio_standalone INTERFACE "${_asio_include_dir}")
        target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
        add_library(asio::asio ALIAS asio_standalone)
    endif()

    if(TARGET asio::asio AND WIN32)
        target_compile_definitions(asio_standalone INTERFACE _WIN32_WINNT=0x0A00)
    endif()
    if(ARG_REQUIRED)
        _coroutine_study_require(asio::asio "COROUTINE_STUDY_ENABLE_ASIO")
    endif()
endfunction()

function(coroutine_study_setup_cppcoro)
    cmake_parse_arguments(ARG "REQUIRED" "" "" ${ARGN})
    if(TARGET cppcoro::cppcoro)
        return()
    endif()

    find_package(cppcoro QUIET CONFIG)
    _coroutine_study_bridge_target(cppcoro::cppcoro cppcoro)

    if(NOT TARGET cppcoro::cppcoro AND COROUTINE_STUDY_FETCH_DEPS)
        FetchContent_Declare(
            cppcoro
            GIT_REPOSITORY https://github.com/andreasbuhr/cppcoro.git
            GIT_TAG        8642e98596a92be30a2b061d3ed306d959d3214e
            GIT_SHALLOW    FALSE
            GIT_PROGRESS   TRUE
        )
        set(CPPCORO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(_coroutine_study_saved_build_testing "${BUILD_TESTING}")
        set(BUILD_TESTING OFF)
        FetchContent_MakeAvailable(cppcoro)
        set(BUILD_TESTING "${_coroutine_study_saved_build_testing}")
        _coroutine_study_bridge_target(cppcoro::cppcoro cppcoro)
    endif()

    if(ARG_REQUIRED)
        _coroutine_study_require(cppcoro::cppcoro "COROUTINE_STUDY_ENABLE_CPPCORO")
    endif()
endfunction()

function(coroutine_study_setup_liburing)
    cmake_parse_arguments(ARG "REQUIRED" "" "" ${ARGN})
    if(TARGET liburing::liburing)
        return()
    endif()
    if(WIN32)
        if(ARG_REQUIRED)
            message(FATAL_ERROR "COROUTINE_STUDY_ENABLE_IO_URING is Linux-only.")
        endif()
        return()
    endif()

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBURING QUIET IMPORTED_TARGET liburing>=2.15)
        if(TARGET PkgConfig::LIBURING)
            add_library(liburing::liburing ALIAS PkgConfig::LIBURING)
        endif()
    endif()
    if(ARG_REQUIRED)
        _coroutine_study_require(liburing::liburing "COROUTINE_STUDY_ENABLE_IO_URING")
    endif()
endfunction()

function(coroutine_study_setup_folly)
    cmake_parse_arguments(ARG "REQUIRED" "" "" ${ARGN})
    if(TARGET folly::folly)
        return()
    endif()
    find_package(Folly QUIET CONFIG)
    _coroutine_study_bridge_target(folly::folly Folly::folly)
    if(ARG_REQUIRED)
        _coroutine_study_require(folly::folly "COROUTINE_STUDY_ENABLE_FOLLY")
    endif()
endfunction()

function(coroutine_study_setup_cobalt)
    cmake_parse_arguments(ARG "REQUIRED" "" "" ${ARGN})
    if(TARGET Boost::cobalt)
        return()
    endif()
    find_package(Boost 1.92 QUIET CONFIG COMPONENTS cobalt)
    if(ARG_REQUIRED)
        _coroutine_study_require(Boost::cobalt "COROUTINE_STUDY_ENABLE_COBALT")
    endif()
endfunction()

function(coroutine_study_link_stage3_deps target)
    if(target MATCHES "^H[123]_|^Capstone5_|^mini_as_awaitable_test$")
        target_link_libraries(${target} PRIVATE stdexec::stdexec)
    elseif(target STREQUAL "I1_asio_echo" OR target STREQUAL "Capstone4_rpc_framework")
        target_link_libraries(${target} PRIVATE asio::asio)
    elseif(target STREQUAL "I2_io_uring_iocp" AND TARGET liburing::liburing)
        target_link_libraries(${target} PRIVATE liburing::liburing)
    elseif(target STREQUAL "I3_folly_safe_task")
        target_link_libraries(${target} PRIVATE folly::folly)
    elseif(target STREQUAL "I4_cobalt_channel")
        target_link_libraries(${target} PRIVATE Boost::cobalt)
    endif()
endfunction()
