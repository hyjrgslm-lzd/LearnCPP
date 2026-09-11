// =====================================================================
// 练习 C-2：when_all / when_any 的并行组合
// 文档参考：C09_Coroutines/04-模块C-取消与组合.md  -> 练习 C-2
// 官方参考：
//   - Lewis Baker, "Structured Concurrency" CppCon 2019
//   - cppcoro when_all.hpp / when_any.hpp 设计
//   - P2300R10: when_all 与三通道完成语义
// 学习要点：
//   1. when_all 汇合：所有子 task 完成后取一个 tuple。
//   2. when_any 竞速：第一个完成者决定结果，其余应被取消。
//   3. 超时模式 = when_any(real_task, timeout_after(Nms))。
// =====================================================================
#include "coroutine_study/lazy_task.hpp"
#include "coroutine_study/exercise_check.hpp"

#include <atomic>
#include <exception>
#include <array>
#include <chrono>
#include <coroutine>
#include <iostream>
#include <mutex>
#include <print>
#include <stop_token>
#include <thread>
#include <tuple>
#include <vector>

using namespace std::chrono_literals;

struct timeout_marker { static constexpr int value = -1; };

// ---------------------------------------------------------------------
// 异步延迟：sleep + 立即 resume。学习版，多 task 并发时建议每个 await
// 起一条 jthread 来驱动，避免阻塞同一调用线程。
// ---------------------------------------------------------------------
struct async_sleep {
    std::chrono::milliseconds dur;
    std::stop_token st{};
    bool await_ready() const noexcept { return dur <= 0ms || st.stop_requested(); }
    void await_suspend(std::coroutine_handle<> h) const {
        std::this_thread::sleep_for(dur);
        h.resume();
    }
    void await_resume() const noexcept {}
};

// ---------------------------------------------------------------------
// 三种模拟 fetch：用不同延迟模拟"缓存/数据库/远程"。
// ---------------------------------------------------------------------
coroutine_study::lazy_task<int> fetch_cache(std::stop_token st = {}) {
    co_await async_sleep{50ms, st};
    if (st.stop_requested()) co_return timeout_marker::value;
    co_return 100;
}
coroutine_study::lazy_task<int> fetch_db(std::stop_token st = {}) {
    co_await async_sleep{150ms, st};
    if (st.stop_requested()) co_return timeout_marker::value;
    co_return 200;
}
coroutine_study::lazy_task<int> fetch_remote(std::stop_token st = {}) {
    co_await async_sleep{300ms, st};
    if (st.stop_requested()) co_return timeout_marker::value;
    co_return 300;
}

struct start_gate {
    int expected = 0;
    std::atomic<int> started{0};
    std::atomic<int> completed{0};
    std::atomic<bool> released{false};
    std::mutex mutex;
    std::array<std::coroutine_handle<>, 3> handles{};
};

struct gated_awaitable {
    start_gate& gate;
    int index = 0;

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> h) {
        std::array<std::coroutine_handle<>, 3> to_resume{};
        int count = 0;
        {
            std::lock_guard lock(gate.mutex);
            gate.handles[index] = h;
            count = gate.started.fetch_add(1, std::memory_order_acq_rel) + 1;
            if (count == gate.expected) {
                gate.released.store(true, std::memory_order_release);
                to_resume = gate.handles;
            }
        }
        if (count == gate.expected) {
            for (int i = 0; i < gate.expected; ++i) {
                if (i != index && to_resume[i]) to_resume[i].resume();
            }
            return false;
        }
        return true;
    }

    void await_resume() const {
        coroutine_study::check(gate.released.load(std::memory_order_acquire),
                               "Part 2: gated child resumed before all siblings reached the gate");
        gate.completed.fetch_add(1, std::memory_order_acq_rel);
    }
};

coroutine_study::lazy_task<int> gated_fetch(start_gate& gate, int index, int value) {
    co_await gated_awaitable{gate, index};
    co_return value;
}

// ---------------------------------------------------------------------
// 简化版 when_all：接受 3 个 lazy_task<int>，返回 tuple<int,int,int>。
// 重点是"语义"而非"高性能实现"。
// ---------------------------------------------------------------------
template <typename T1, typename T2, typename T3>
coroutine_study::lazy_task<std::tuple<int, int, int>>
when_all(T1 t1, T2 t2, T3 t3) {
    // TODO [必做 1]：并行启动 3 个 task（可以为每个 task 起一条 jthread
    //   分别 .get()，再用 atomic 计数 + 条件变量等齐结果），
    //   最后 co_return std::make_tuple(r1, r2, r3)。
    //
    //   也可以用单线程 run_loop 先启动所有 child，再由事件循环逐个恢复。
    //   未完成时必须有限失败，不能保留串行 sync_wait 占位，否则 gated child 会挂住。
    (void)t1;
    (void)t2;
    (void)t3;
    throw std::logic_error{"TODO[必做 1]: implement when_all by starting all child tasks before waiting"};
    co_return std::tuple<int, int, int>{};
}

// ---------------------------------------------------------------------
// 简化版 when_any：返回第一个完成者的整数结果（用 -1 标记超时占位）。
// 真实实现应通知未完成 task 取消。
// ---------------------------------------------------------------------
coroutine_study::lazy_task<int> timeout_after(std::chrono::milliseconds d, std::stop_token st = {}) {
    co_await async_sleep{d, st};
    co_return timeout_marker::value;
}

template <typename TA, typename TB>
coroutine_study::lazy_task<int> when_any(std::stop_source& stop_source, TA ta, TB tb) {
    // TODO [必做 2]：并行驱动 ta / tb，
    //   用 atomic_flag 选第一个完成者，立刻 co_return 它的值；
    //   并调用 stop_source.request_stop() 通知另一方在下一个检查点停止。
    //   最后 join loser，确保资源收束。
    //
    //   也可以用单线程事件循环实现竞速；正确性来自先启动双方和收束 loser，
    //   不是来自线程 ID 或 sleep 耗时。
    (void)stop_source;
    (void)ta;
    (void)tb;
    throw std::logic_error{"TODO[必做 2]: implement when_any by starting both branches before selecting the winner"};
    co_return 0;
}

int main() try {
    std::println("===== 练习 C-2：when_all / when_any =====\n");

    // ------------------ when_all 汇合 ------------------
    {
        start_gate gate;
        gate.expected = 3;
        std::stop_source all_src;
        auto t = when_all(gated_fetch(gate, 0, 100),
                          gated_fetch(gate, 1, 200),
                          gated_fetch(gate, 2, 300));
        auto [a, b, c] = coroutine_study::sync_wait(std::move(t));
        std::println("[when_all] cache={} db={} remote={} gated_started={} gated_completed={}",
                     a, b, c, gate.started.load(), gate.completed.load());
        // Part 4 观察入口：50/150/300ms 可用于手工观察串行与 fan-out 的差异；
        //   当前学生检查只用 gate 证明“先启动全部子任务，再释放完成”。
        coroutine_study::check(
            std::make_tuple(a, b, c) == std::make_tuple(100, 200, 300),
            "Part 1/2: when_all preserves cache/db/remote result slots"
        );
        coroutine_study::check(gate.started == 3 && gate.completed == 3 && gate.released,
                               "Part 2/3: when_all must start all children before releasing any child");
    }

    // ------------------ when_any 超时 ------------------
    {
        std::stop_source any_src;
        start_gate gate;
        gate.expected = 2;
        auto t = when_any(any_src,
                          gated_fetch(gate, 0, 300),
                          gated_fetch(gate, 1, timeout_marker::value));
        int v = coroutine_study::sync_wait(std::move(t));
        if (v == timeout_marker::value) {
            std::println("[when_any] timeout branch won after both gated branches started");
        } else {
            std::println("[when_any] remote 先完成 = {}", v);
        }
        coroutine_study::check(gate.started == 2 && gate.released,
                               "Part 3: when_any must start both branches before selecting a winner");
        // TODO [必做 4]：把 200ms 改成 400ms，重跑一次，
        //   观察"超时优先" -> "数据优先"的切换。
        std::stop_source slower_timeout_src;
        auto observation = when_any(slower_timeout_src,
                                    fetch_remote(slower_timeout_src.get_token()),
                                    timeout_after(400ms, slower_timeout_src.get_token()));
        int observation_value = coroutine_study::sync_wait(std::move(observation));
        std::println("[when_any] 400ms observation value = {}", observation_value);
    }

    // ------------------ 进阶 ------------------
    // TODO [进阶 1]：when_all 中某个 task 抛异常，
    //   设计如何向其余 task 发起 request_stop 并把异常向上传播。
    // TODO [进阶 2]：when_any 中第一个完成者出来后，
    //   主动取消其余 task。验证它们能在下一个 co_await 点跳出。
    // TODO [进阶 3]：用 when_all 组合 when_any，做"3 组请求每组带超时"的嵌套结构。

    std::println("\n===== Done =====");
    return 0;
}
catch (const std::exception& e) {
    std::cerr << "student check failed: " << e.what() << '\n';
    return 1;
}
catch (...) {
    std::cerr << "student check failed: unknown exception\n";
    return 1;
}
