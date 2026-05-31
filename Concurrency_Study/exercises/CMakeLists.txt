cmake_minimum_required(VERSION 3.25)
project(concurrency_study_exercises LANGUAGES CXX)

# ============================================================
# C++ 标准
#   基线 C++20：jthread / stop_token / latch / barrier / semaphore /
#   atomic_ref / atomic wait·notify / atomic<shared_ptr> 全部为 C++20。
#   个别题目（如模块 K 讲 std::simd 标准 API）会在自己的 CMakeLists.txt
#   里按需提升到 C++23/26 或切换到回退库。
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC（主战场 VS2026）：__cplusplus 正确报告 + UTF-8 源码
if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /EHsc)
endif()

# 让所有子项目都能 include 到 cmake/ 下的 *Setup.cmake 以及共享头
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CONCURRENCY_STUDY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "共享头目录")

# ============================================================
# 阶段一 · 并发基础（会用）—— 纯标准库，零外部依赖
# ============================================================
add_subdirectory(A1_jthread_lifecycle)
add_subdirectory(A2_stop_token_cancellation)
add_subdirectory(A3_thread_exception)
add_subdirectory(B1_mutex_family)
add_subdirectory(B2_deadlock_scoped_lock)
add_subdirectory(B3_call_once)
add_subdirectory(C1_condvar_predicate)
add_subdirectory(C2_bounded_queue_condvar)
add_subdirectory(C3_interruptible_wait)
add_subdirectory(D1_promise_future)
add_subdirectory(D2_async_policies)
add_subdirectory(D3_packaged_task)
add_subdirectory(Capstone1_thread_pool)

# ============================================================
# 阶段二 · 原子与内存模型（懂底层）—— 纯标准库 + include/ 自带教学实现
# ============================================================
add_subdirectory(E1_atomic_basics)
add_subdirectory(E2_atomic_ref)
add_subdirectory(E3_cas_loop)
add_subdirectory(F1_release_acquire)
add_subdirectory(F2_relaxed_counter)
add_subdirectory(F3_seqcst_fence)
add_subdirectory(F4_publish_pattern)
add_subdirectory(G1_treiber_stack)
add_subdirectory(G2_aba_problem)
add_subdirectory(G3_spsc_ringbuffer)
add_subdirectory(H1_latch_barrier)
add_subdirectory(H2_semaphore)
add_subdirectory(H3_atomic_wait_notify)
add_subdirectory(I1_atomic_shared_ptr)
add_subdirectory(I2_hazard_pointer)
add_subdirectory(I3_rcu)
add_subdirectory(Capstone2_lockfree_queue)

# ============================================================
# 阶段三 · HPC（工程价值）
#   模块 J：纯标准库
#   模块 K：xsimd（FetchContent，header-only，MSVC 友好）
#   模块 L：MSVC 内置并行 STL / 非 MSVC 经 TbbSetup 引入 TBB
#   模块 M：work-stealing 自实现 + 可选 stdexec 桥接
# ============================================================
add_subdirectory(J1_false_sharing)
add_subdirectory(J2_interference_size)
add_subdirectory(J3_numa_concept)
add_subdirectory(K1_simd_basics)
add_subdirectory(K2_simd_dotproduct)
add_subdirectory(K3_simd_where_select)
add_subdirectory(L1_par_algorithms)
add_subdirectory(L2_parallel_reduce_scan)
add_subdirectory(L3_par_vs_seq_bench)
add_subdirectory(M1_work_stealing_pool)
add_subdirectory(Capstone3_parallel_compute)

# ------------------------------------------------------------
# M2_execution_bridge 默认关闭：
#   它依赖 NVIDIA/stdexec（P2300 参考实现），而 stdexec 的 CMake 会
#   经 FetchContent 拖入重型的 rapids-cmake 构建依赖，首次配置耗时长、
#   且强依赖网络（github）。本套对 senders 仅作“桥接演示”，深入见
#   Execution_Study\，故默认不构建它，避免拖累其余 41 个目标的配置。
#
#   需要构建 M2 时显式开启（需网络或 third_party/stdexec 预克隆）：
#       cmake --preset vs2026 -DCONCURRENCY_STUDY_ENABLE_STDEXEC=ON
# ------------------------------------------------------------
option(CONCURRENCY_STUDY_ENABLE_STDEXEC
       "构建 M2_execution_bridge（需要 NVIDIA/stdexec，依赖较重）" OFF)
if(CONCURRENCY_STUDY_ENABLE_STDEXEC)
    add_subdirectory(M2_execution_bridge)
else()
    message(STATUS "[concurrency_study] M2_execution_bridge 已跳过（默认）。"
                   " 开启：-DCONCURRENCY_STUDY_ENABLE_STDEXEC=ON")
endif()
