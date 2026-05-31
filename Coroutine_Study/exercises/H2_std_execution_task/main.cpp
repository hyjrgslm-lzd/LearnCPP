// H-2 用 std::execution::task<T>（P3552R3）
// 文档参考：10-模块H-协程与sender_receiver桥接.md 「练习 H-2」
// 官方参考：
//   - P3552R3 std::execution::task<T>
//   - P2300R10 std::execution
//   - stdexec/exec/task.hpp 实际实现
//
// 目标：使用 P3552R3 task（如可用）或 stdexec 的等价 task，体验：
//      - lazy 启动 + scheduler-aware
//      - co_await schedule(sch) 切换调度器
//      - co_await when_all 并行组合
//      - 通过 promise.get_env() 暴露 scheduler

#include <stdexec/execution.hpp>

// 守卫：标准 <execution> 的 P3552 task 仅在新 std lib 中可用。
// 当前主流 toolchain 还未实现 P3552，本题主线走 stdexec exec::task 等价路径。
#if defined(__cpp_lib_execution) && __cpp_lib_execution >= 202902L
  #include <execution>
  namespace task_ns = std::execution;
#else
  // 回退：stdexec 在 exec::task 提供同语义实现
  #include <exec/task.hpp>
  namespace task_ns = exec;
#endif

#include <exec/single_thread_context.hpp>

#include <cstdio>
#include <thread>
#include <chrono>

namespace ex = stdexec;

// ============ 测试 1：scheduler 切换 ============
task_ns::task<int> scheduler_aware_compute()
{
    std::printf("[start] thread = %zu\n",
                std::hash<std::thread::id>{}(std::this_thread::get_id()));

    // 获取当前 scheduler（由 sync_wait/when_all 注入）
    auto sch = co_await ex::get_scheduler();

    // co_await schedule(sch) 把后续计算调度到 sch 上
    co_await ex::schedule(sch);

    std::printf("[after schedule] thread = %zu\n",
                std::hash<std::thread::id>{}(std::this_thread::get_id()));

    co_return 42;
}

// ============ 测试 2：when_all + task 并行 ============
task_ns::task<int> fetch_a() {
    std::printf("[fetch_a] thread = %zu\n",
                std::hash<std::thread::id>{}(std::this_thread::get_id()));
    co_return 10;
}

task_ns::task<int> fetch_b() {
    std::printf("[fetch_b] thread = %zu\n",
                std::hash<std::thread::id>{}(std::this_thread::get_id()));
    co_return 20;
}

task_ns::task<int> parallel_fetch() {
    auto [a, b] = co_await ex::when_all(fetch_a(), fetch_b());
    co_return a + b;
}

// ============ main ============
int main()
{
    std::printf("===== H-2: std::execution::task =====\n\n");

    {
        std::printf("--- 测试 1：scheduler-aware compute ---\n");
        // 准备一个 single_thread_context 作为 scheduler 源
        exec::single_thread_context ctx;
        auto sch = ctx.get_scheduler();

        // sync_wait 默认环境里没有 scheduler——把 scheduler 注入
        // stdexec 中常用模式：starts_on(sch, task) 或在 sync_wait 处搭建带 scheduler 的 env
        auto result = ex::sync_wait(
            ex::starts_on(sch, scheduler_aware_compute())
        );
        if (result) {
            auto [v] = *result;
            std::printf("  result = %d (expect 42)\n", v);
        }
    }

    {
        std::printf("\n--- 测试 2：when_all 并行组合 ---\n");
        exec::single_thread_context ctx;
        auto sch = ctx.get_scheduler();

        auto result = ex::sync_wait(
            ex::starts_on(sch, parallel_fetch())
        );
        if (result) {
            auto [v] = *result;
            std::printf("  result = %d (expect 30)\n", v);
        }
    }

    // TODO [必做]：在笔记中画出环境传播链：
    //   sync_wait env → connect → task promise env → 子 sender env
    // TODO [必做]：解释 scheduler affinity 如何帮你在 task 中"无锁访问局部变量"。
    // TODO [进阶]：自实现 my_scheduler，让 task 在你的 scheduler 上恢复。
    // TODO [进阶]：为 task 的 get_env 添加 stop_token 转发。

    // 备注：当前 toolchain 大部分还没有 P3552 task；本题以 stdexec exec::task
    //       为等价实现。两者的 scheduler-aware 语义是一致的。
    std::printf("\n===== Done =====\n");
    return 0;
}
