// =====================================================================
// 练习 E-3：Trivial Awaitable 与短路优化
// 文档参考：C09_Coroutines/07-模块E-awaitable三层与co_await变换.md  -> 练习 E-3
// 官方参考：
//   - C++20 [expr.await] / [coroutine.trivial.awaitables]
//   - Lewis Baker, "C++ coroutines: Understanding the co_await operator"
//   - Gor Nishanov, "HALO: Heap Allocation eLision Optimization" CppCon 2018
//   - cppreference: std::suspend_never / std::suspend_always
// 学习要点：
//   1. await_ready -> true 让 co_await 退化为对 await_resume 的直接调用。
//   2. 优化器可能消除不可达挂起分支；这不是固定优化级别保证。
//   3. ready 短路与 HALO 分别验证：前者控制 await_suspend，后者控制 frame allocation。
// =====================================================================
#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <print>
#include <string>
#include <utility>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------
// simple_task：本练习的最小 task，不追踪 continuation
// ---------------------------------------------------------------------
struct simple_task {
    struct promise_type {
        simple_task get_return_object() {
            return simple_task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }
    };
    std::coroutine_handle<promise_type> h_;
    explicit simple_task(std::coroutine_handle<promise_type> h) : h_(h) {}
    simple_task(const simple_task&) = delete;
    simple_task(simple_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~simple_task() { if (h_) h_.destroy(); }
    void run_to_end() {
        h_.resume();
        // simple_task 用 suspend_always 作 initial_suspend，body 中没有
        // co_await suspend_always 这种"故意挂起"时可一次跑完。
    }
};

// ---------------------------------------------------------------------
// always_ready：典型 trivial awaitable
//   await_ready 永远 true -> 编译器在 -O2 下可消除挂起路径
// ---------------------------------------------------------------------
struct always_ready {
    // TODO [必做 1]：返回 true，让 await_suspend 永不被调用。
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {
        // 永远不会被执行（保留是为了满足当前 C++23 协程规范）
    }
    int  await_resume() const noexcept { return 42; }
};

// ---------------------------------------------------------------------
// conditional_ready：按运行时参数决定是否进入 await_suspend；本例随后返回 false 继续
// ---------------------------------------------------------------------
struct conditional_ready {
    bool should_suspend;
    bool await_ready() const noexcept {
        // TODO [必做 2]：should_suspend 为 false 时返回 true，跳过挂起。
        return !should_suspend;
    }
    bool await_suspend(std::coroutine_handle<>) const noexcept {
        std::println("    [conditional] await_suspend reached; return false to continue safely");
        return false;
    }
    int await_resume() const noexcept {
        return should_suspend ? 1 : 0;
    }
};

// ---------------------------------------------------------------------
// trivial_awaitable：标准三方法接口；计数验证 await_suspend 不执行
// ---------------------------------------------------------------------
struct trivial_awaitable {
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {
        // await_ready 恒真时不执行，但当前标准仍要求表达式良构。
    }
    std::string await_resume() const { return "trivial-result"; }
};

// ---------------------------------------------------------------------
// 协程：连续多次 co_await always_ready
// ---------------------------------------------------------------------
simple_task many_noop_co_awaits() {
    std::println("  [task] before co_await 1");
    int v1 = co_await always_ready{};
    std::println("  [task] after  co_await 1, v1 = {}", v1);
    int v2 = co_await always_ready{};
    std::println("  [task] after  co_await 2, v2 = {}", v2);
    int v3 = co_await always_ready{};
    std::println("  [task] after  co_await 3, v3 = {}", v3);
    co_return;
}

simple_task with_conditional() {
    std::println("  [task] co_await conditional_ready{{false}}:");
    int v = co_await conditional_ready{false};
    std::println("    => {} (期望 0，未挂起)", v);
    std::println("  [task] co_await conditional_ready{{true}}:");
    int w = co_await conditional_ready{true};
    std::println("    => {} (期望 1，调用 await_suspend，但 bool=false 不保持挂起)", w);
    co_return;
}

simple_task with_trivial() {
    auto s = co_await trivial_awaitable{};
    std::println("  [task] trivial_awaitable result = {}", s);
    co_return;
}

// ---------------------------------------------------------------------
// 性能对比版本（给进阶任务用）
// ---------------------------------------------------------------------
simple_task version_A_trivial() {
    co_await always_ready{};
    co_await always_ready{};
    co_return;
}

simple_task version_B_suspend() {
    // TODO [必做 3]：用 std::suspend_always 触发完整挂起路径，
    //   对照 always_ready 的 fast path。
    //   注意：这会让协程真正挂起 —— 演示版只展示一次启动，不再驱动。
    co_await std::suspend_never{}; // 占位：避免协程真的卡住
    co_await std::suspend_never{};
    co_return;
}

int main() {
    std::println("===== 练习 E-3：Trivial Awaitable =====\n");

    std::println("--- 实验 1：连续多次 always_ready ---");
    {
        auto t = many_noop_co_awaits();
        t.run_to_end();
    }
    std::println("");

    std::println("--- 实验 2：conditional_ready ---");
    {
        auto t = with_conditional();
        t.run_to_end();
    }
    std::println("");

    std::println("--- 实验 3：标准三方法 trivial_awaitable ---");
    {
        auto t = with_trivial();
        t.run_to_end();
    }
    std::println("");

    // ------------------ 进阶 ------------------
    // TODO [必做 4]：用 -O2 + Godbolt 观察 always_ready 版本是否还有 operator new。
    // TODO [进阶 1]：测量 1e7 次 always_ready vs suspend_always 的耗时差。
    // TODO [进阶 2]：写"缓存命中不挂起，未命中挂起"的 awaiter，
    //   观察 fast path 与普通函数调用的等价性。
    // TODO [进阶 3]：尝试让 await_ready 取决于 volatile 变量，
    //   观察编译器无法优化的反例。

    std::println("===== Done =====");
    return 0;
}
