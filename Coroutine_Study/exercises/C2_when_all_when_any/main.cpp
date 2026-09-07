// =====================================================================
// 练习 C-2：when_all / when_any 的并行组合
// 文档参考：Coroutine_Study/04-模块C-取消与组合.md  -> 练习 C-2
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

#include <atomic>
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
    //   骨架占位：当前直接顺序 sync_wait，请改为并行版本。
    int r1 = coroutine_study::sync_wait(std::move(t1));
    int r2 = coroutine_study::sync_wait(std::move(t2));
    int r3 = coroutine_study::sync_wait(std::move(t3));
    co_return std::make_tuple(r1, r2, r3);
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
    //   骨架占位：直接等待 ta，请改写为真正竞速并收束 loser。
    int v = coroutine_study::sync_wait(std::move(ta));
    stop_source.request_stop();
    (void)tb; // unused in skeleton
    co_return v;
}

int main() {
    std::println("===== 练习 C-2：when_all / when_any =====\n");

    // ------------------ when_all 汇合 ------------------
    {
        auto start = std::chrono::steady_clock::now();
        std::stop_source all_src;
        auto t = when_all(fetch_cache(all_src.get_token()),
                          fetch_db(all_src.get_token()),
                          fetch_remote(all_src.get_token()));
        auto [a, b, c] = coroutine_study::sync_wait(std::move(t));
        auto dur = std::chrono::steady_clock::now() - start;
        std::println("[when_all] cache={} db={} remote={} 总耗时 {}ms",
                     a, b, c,
                     std::chrono::duration_cast<std::chrono::milliseconds>(dur).count());
        // TODO [必做 3]：并行版应在 ~300ms 内完成；
        //   骨架的串行版本会接近 50+150+300=500ms。
    }

    // ------------------ when_any 超时 ------------------
    {
        std::stop_source any_src;
        auto t = when_any(any_src,
                          fetch_remote(any_src.get_token()),
                          timeout_after(200ms, any_src.get_token()));
        int v = coroutine_study::sync_wait(std::move(t));
        if (v == timeout_marker::value) {
            std::println("[when_any] 超时（remote 在 200ms 内未完成）");
        } else {
            std::println("[when_any] remote 先完成 = {}", v);
        }
        // TODO [必做 4]：把 200ms 改成 400ms，重跑一次，
        //   观察"超时优先" -> "数据优先"的切换。
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
