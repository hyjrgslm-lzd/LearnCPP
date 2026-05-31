cmake_minimum_required(VERSION 3.28)
project(coroutine_study_exercises LANGUAGES CXX)

# ============================================================
# C++ 标准（协程 + std::generator 需要 C++23/26）
# ============================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: 启用 __cplusplus 正确报告 + UTF-8 源码 + 标准 conformant 预处理器
# 同时通过 /std:c++latest 启用最新 C++26 特性预览（CMake 4.x 在 MSVC 下尚未稳定识别 CXX_STANDARD 26）
if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /Zc:preprocessor /std:c++latest)
endif()
# GCC/Clang 通过 -std=c++2c 启用 C++26 预览
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-std=c++2c)
endif()

# ============================================================
# 引入第三方依赖（stage1/2 基本不依赖；stage3 需要 stdexec / asio / liburing / folly / cobalt）
# ============================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmake/ThirdPartySetup.cmake)

# ============================================================
# stage 3 闸门：默认开启，但首次配置较慢；可以 -DCOROUTINE_STUDY_ENABLE_STAGE3=OFF 跳过
# ============================================================
option(COROUTINE_STUDY_ENABLE_STAGE3 "Enable stage 3 exercises (modules H/I/J + capstones 4/5)" ON)

# ============================================================
# 第一阶段：模块 A / B / C + 第一阶段结课
# ============================================================
add_subdirectory(A1_first_generator)
add_subdirectory(A2_co_return_lazy_task)
add_subdirectory(A3_co_await_future)

add_subdirectory(B1_recursive_generator)
add_subdirectory(B2_task_sequential)
add_subdirectory(B3_callback_to_awaiter)

add_subdirectory(C1_stop_token_cancel)
add_subdirectory(C2_when_all_when_any)
add_subdirectory(C3_async_scope)

add_subdirectory(Capstone1_async_crawler)

# ============================================================
# 第二阶段：模块 D / E / F / G
# ============================================================
add_subdirectory(D1_promise_8_hooks)
add_subdirectory(D2_eager_vs_lazy)
add_subdirectory(D3_final_suspend_symmetric)

add_subdirectory(E1_co_await_lookup)
add_subdirectory(E2_await_suspend_three)
add_subdirectory(E3_trivial_awaitable)

add_subdirectory(F1_frame_layout)
add_subdirectory(F2_promise_allocator)
add_subdirectory(F3_halo_diagnose)

add_subdirectory(G1_shared_task)
add_subdirectory(G2_when_all_impl)
add_subdirectory(G3_sync_wait_impl)

# ============================================================
# 第三阶段：模块 H / I / J + 第三阶段两个结课
# ============================================================
if(COROUTINE_STUDY_ENABLE_STAGE3)
    add_subdirectory(H1_as_awaitable)
    add_subdirectory(H2_std_execution_task)
    add_subdirectory(H3_bidirectional_bridge)

    add_subdirectory(I1_asio_echo)
    add_subdirectory(I2_io_uring_iocp)
    if(NOT WIN32)
        add_subdirectory(I3_folly_safe_task)
        add_subdirectory(I4_cobalt_channel)
    endif()

    add_subdirectory(J1_eight_pitfalls)
    add_subdirectory(J2_cross_compiler_abi)
    add_subdirectory(J3_coroutine_tracing)

    add_subdirectory(Capstone4_rpc_framework)
    add_subdirectory(Capstone5_mini_corolib)
endif()
