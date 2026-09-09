cmake_minimum_required(VERSION 3.25)
project(C10_Execution LANGUAGES CXX)

# ============================================================
# C++ 标准
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: 启用 __cplusplus 正确报告 + UTF-8 源码
if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8)
endif()

# ============================================================
# 引入 stdexec（FetchContent 或本地）
# ============================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(StdexecSetup)

# ============================================================
# 第一阶段：框架使用与概念理解
# ============================================================
add_subdirectory(A1_two_executions)
add_subdirectory(A2_value_channel)
add_subdirectory(A3_fan_out_merge)
add_subdirectory(B4_scheduler_capability)
add_subdirectory(B5_context_switch)
add_subdirectory(B6_pipeline)
add_subdirectory(C1_7_error_channel)
add_subdirectory(C1_8_cancellation)
add_subdirectory(C2_9_environment)
add_subdirectory(C2_10_scope_lifetime)
add_subdirectory(D11_minimal_receiver)
add_subdirectory(D12_minimal_sender)
add_subdirectory(D13_sender_adaptor)
add_subdirectory(D14_coroutine_bridge)

# ============================================================
# 第二阶段：实现技巧与架构设计
# ============================================================
add_subdirectory(E1_cpo_niebloid)
add_subdirectory(E2_tag_invoke)
add_subdirectory(E3_evolution_compare)
add_subdirectory(F1_type_list)
add_subdirectory(F2_completion_signatures)
add_subdirectory(F3_concepts)
add_subdirectory(G1_my_then)
add_subdirectory(G2_pipe_syntax)
add_subdirectory(G3_retry)
add_subdirectory(H1_run_loop)
add_subdirectory(H2_custom_query)
add_subdirectory(H3_coroutine_task)
