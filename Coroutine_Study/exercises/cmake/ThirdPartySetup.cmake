# exercises/cmake/ThirdPartySetup.cmake
#
# 统一的 stage 3 第三方依赖引入逻辑。
# - stage 1/2（模块 A/B/C/D/E/F/G）：不需要任何第三方依赖，仅 C++23 <generator> + 随附 lazy_task.hpp。
# - stage 3：需要 stdexec / asio / liburing / folly / cobalt。
#
# 当子目录作为独立项目打开时（VS Code 打开单个习题），
# 由该子目录的 CMakeLists.txt include 此文件来拉取依赖；
# 当子目录作为顶层项目的子目录时，顶层已经 include 过本文件，重复 include 会被守卫跳过。

include_guard(GLOBAL)

include(FetchContent)

# ============================================================
# stdexec —— stage 3 模块 H 协程 ↔ sender/receiver 桥接
# ============================================================
function(_coroutine_study_setup_stdexec)
    if(TARGET stdexec::stdexec)
        return()
    endif()
    if(TARGET STDEXEC::stdexec)
        if(NOT TARGET stdexec::stdexec)
            add_library(stdexec::stdexec ALIAS stdexec)
        endif()
        return()
    endif()

    find_package(stdexec QUIET CONFIG)
    if(stdexec_FOUND)
        message(STATUS "[ThirdPartySetup] stdexec found via find_package.")
        return()
    endif()

    message(STATUS "[ThirdPartySetup] stdexec not found — using FetchContent...")
    FetchContent_Declare(
        stdexec
        GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
        # 钉到已验证可用的 commit（MSVC 14.51 / C++26 实测编译通过）。
        # 想追最新改回 main 即可。注意：按 SHA 拉取时 GIT_SHALLOW 须为 FALSE
        # （GitHub 默认不支持对任意 commit 的 shallow want）。
        GIT_TAG        02d671da624daafc63dc42f60bfba40f97161400
        GIT_SHALLOW    FALSE
        GIT_PROGRESS   TRUE
    )
    set(STDEXEC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(STDEXEC_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(STDEXEC_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(stdexec)

    if(NOT TARGET stdexec::stdexec)
        add_library(stdexec::stdexec ALIAS stdexec)
    endif()
    message(STATUS "[ThirdPartySetup] stdexec ready.")
endfunction()

# ============================================================
# Asio (standalone) —— stage 3 模块 I-1 真实异步 IO
# Asio 没有原生 CMake 支持，需要手动创建 INTERFACE target 指向其 include。
# ============================================================
function(_coroutine_study_setup_asio)
    if(TARGET asio::asio)
        return()
    endif()

    message(STATUS "[ThirdPartySetup] asio (standalone) — using FetchContent...")
    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        # 钉到已验证可用的 commit（实测随 stdexec 一起编译通过）。想追最新改回 master。
        GIT_TAG        bd500f0a018db9a845ebaaed5c0318343ae9f497
        GIT_SHALLOW    FALSE
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(asio)

    add_library(asio_standalone INTERFACE)
    # asio 仓库新旧布局兼容：master 直接放 include/，老版本放 asio/include/
    if(EXISTS "${asio_SOURCE_DIR}/asio/include/asio.hpp")
        set(_asio_include_dir "${asio_SOURCE_DIR}/asio/include")
    elseif(EXISTS "${asio_SOURCE_DIR}/include/asio.hpp")
        set(_asio_include_dir "${asio_SOURCE_DIR}/include")
    else()
        message(FATAL_ERROR "[ThirdPartySetup] cannot find asio.hpp under ${asio_SOURCE_DIR}")
    endif()
    target_include_directories(asio_standalone INTERFACE "${_asio_include_dir}")
    target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
    if(WIN32)
        target_compile_definitions(asio_standalone INTERFACE _WIN32_WINNT=0x0A00)
    endif()
    add_library(asio::asio ALIAS asio_standalone)

    message(STATUS "[ThirdPartySetup] asio ready.")
endfunction()

# ============================================================
# liburing —— stage 3 模块 I-2（io_uring/IOCP 的 Linux 分支），仅 Linux
# 占位：依赖系统包管理器或源码编译，不在此处自动拉取。
# ============================================================
function(_coroutine_study_setup_liburing)
    if(TARGET liburing::liburing)
        return()
    endif()
    if(WIN32)
        return()  # Windows 不支持 io_uring
    endif()
    # Stage 3 才需要：建议通过 `apt install liburing-dev` 或 `pkg-config --cflags --libs liburing` 提供
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBURING QUIET IMPORTED_TARGET liburing)
        if(TARGET PkgConfig::LIBURING)
            add_library(liburing::liburing ALIAS PkgConfig::LIBURING)
            message(STATUS "[ThirdPartySetup] liburing found via pkg-config.")
            return()
        endif()
    endif()
    message(STATUS "[ThirdPartySetup] liburing NOT found — module I-2 will be skipped at link time.")
endfunction()

# ============================================================
# folly —— stage 3 模块 I-3（Folly SafeTask），仅非 Windows
# 占位：folly 依赖庞杂，建议通过系统包或 vcpkg 提供。
# ============================================================
function(_coroutine_study_setup_folly)
    if(TARGET folly::folly)
        return()
    endif()
    find_package(folly QUIET CONFIG)
    if(folly_FOUND)
        message(STATUS "[ThirdPartySetup] folly found via find_package.")
        return()
    endif()
    message(STATUS "[ThirdPartySetup] folly NOT found — module I-3 will be skipped at link time.")
endfunction()

# ============================================================
# Boost.Cobalt —— stage 3 模块 I-4，仅非 Windows
# 占位：通过 Boost find_package 引入。
# ============================================================
function(_coroutine_study_setup_cobalt)
    if(TARGET Boost::cobalt)
        return()
    endif()
    if(WIN32)
        return()
    endif()
    find_package(Boost QUIET COMPONENTS cobalt)
    if(Boost_FOUND AND TARGET Boost::cobalt)
        message(STATUS "[ThirdPartySetup] Boost.Cobalt found.")
        return()
    endif()
    message(STATUS "[ThirdPartySetup] Boost.Cobalt NOT found — module I-4 will be skipped at link time.")
endfunction()

# ============================================================
# helper：让 stage3 题目子项目一行链接所有可用的 stage3 依赖
# 用法：
#   add_executable(H1_xxx main.cpp)
#   coroutine_study_link_stage3_deps(H1_xxx)
# ============================================================
function(coroutine_study_link_stage3_deps target)
    _coroutine_study_setup_stdexec()
    _coroutine_study_setup_asio()
    _coroutine_study_setup_liburing()
    _coroutine_study_setup_folly()
    _coroutine_study_setup_cobalt()

    if(TARGET stdexec::stdexec)
        target_link_libraries(${target} PRIVATE stdexec::stdexec)
    endif()
    if(TARGET asio::asio)
        target_link_libraries(${target} PRIVATE asio::asio)
    endif()
    if(TARGET liburing::liburing)
        target_link_libraries(${target} PRIVATE liburing::liburing)
    endif()
    if(TARGET folly::folly)
        target_link_libraries(${target} PRIVATE folly::folly)
    endif()
    if(TARGET Boost::cobalt)
        target_link_libraries(${target} PRIVATE Boost::cobalt)
    endif()
endfunction()
