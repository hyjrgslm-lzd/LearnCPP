// =====================================================================
// 练习 C-1：协作式取消 (stop_token + lazy_task)
// 文档参考：C09_Coroutines/04-模块C-取消与组合.md  -> 练习 C-1
// 官方参考：
//   - cppreference <stop_token>: https://en.cppreference.com/w/cpp/thread/stop_token
//   - P2300R10 std::execution: set_stopped 完成通道
//   - Lewis Baker, "Structured Concurrency" CppCon 2019
// 学习要点：
//   1. 取消是协作式的——必须在 co_await 点主动检查 stop_token。
//   2. stopped 表示"有意识地终止"，与 error 路径分离。
//   3. 纯 CPU 循环若不检查 token，取消请求将被忽略。
// =====================================================================
#include "coroutine_study/lazy_task.hpp"
#include "coroutine_study/exercise_check.hpp"

#include <chrono>
#include <exception>
#include <coroutine>
#include <iostream>
#include <print>
#include <stop_token>
#include <thread>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------
// 一个最小的 async_sleep awaiter：在 await_suspend 中阻塞睡眠后立刻 resume。
// 真实工程中应该交给定时器线程，这里只是学习用。
// ---------------------------------------------------------------------
struct async_sleep {
    std::chrono::milliseconds dur;

    bool await_ready() const noexcept { return dur <= 0ms; }
    void await_suspend(std::coroutine_handle<> h) const {
        std::this_thread::sleep_for(dur);
        h.resume();
    }
    void await_resume() const noexcept {}
};

// ---------------------------------------------------------------------
// 模拟"分批处理"协程：每批之间检查 stop_token，被取消则提前 co_return
// 已处理批次数（走 stopped 路径，不抛异常）。
// ---------------------------------------------------------------------
coroutine_study::lazy_task<int>
batch_process(std::stop_token st, int total_batches) {
    int processed = 0;
    for (int i = 0; i < total_batches; ++i) {
        // TODO [必做 1]：在 co_await 之前检查 st.stop_requested()，
        //   若被取消则 co_return processed 提前返回，让调用侧
        //   以 stopped 语义看到部分结果。
        //
        //   提示：if (st.stop_requested()) { co_return processed; }
        if (st.stop_requested()) co_return processed;

        // 必做 2（已给默认实现）：co_await 一个 50ms 的 async_sleep，模拟 I/O 延迟，
        //   也是天然的 cancellation point。保留它，骨架开箱即可观察到挂起；
        //   配合上面必做 1 的 stop 检查，就能跑出协作式取消。
        co_await async_sleep{50ms};

        // TODO [必做 2.b]：在 co_await 之后再次检查 token。
        //   取消可能发生在等待期间，此时提前返回已完成批次数。
        if (st.stop_requested()) co_return processed;

        ++processed;
        std::println("  [batch_process] 完成第 {} 批", processed);
    }
    co_return processed;
}

// ---------------------------------------------------------------------
// 对比版：把取消映射成异常路径（仅用于对比）。
// 完成后请记录两种路径在调用侧的差异（try/catch 强制 vs. 普通取值）。
// ---------------------------------------------------------------------
struct task_cancelled : std::exception {
    const char* what() const noexcept override { return "task cancelled"; }
};

coroutine_study::lazy_task<int>
batch_process_throwing(std::stop_token st, int total_batches) {
    int processed = 0;
    for (int i = 0; i < total_batches; ++i) {
        // TODO [必做 3.a]：co_await 前检查取消，取消后 throw task_cancelled{}。
        if (st.stop_requested()) throw task_cancelled{};
        co_await async_sleep{50ms};
        // TODO [必做 3.b]：co_await 后再次检查取消，对比普通返回路径的 API 差异。
        if (st.stop_requested()) throw task_cancelled{};
        ++processed;
    }
    co_return processed;
}

int main() try {
    std::println("===== 练习 C-1：stop_token 协作式取消 =====\n");

    // ------------------ 主流程 ------------------
    std::stop_source src;
    auto token = src.get_token();

    // TODO [必做 4]：启动 task；在另一线程或 sleep 一段时间后
    //   调用 src.request_stop()，让协程在 co_await 点观察到取消。
    //
    //   建议结构：
    //     auto t = batch_process(token, 10);
    //     std::jthread canceller([&]{
    //         std::this_thread::sleep_for(120ms);
    //         src.request_stop();
    //     });
    //     int done = t.get();
    //     std::println("已处理批次：{}", done);
    auto t = batch_process(token, 10);
    std::jthread canceller([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        src.request_stop();
    });
    int done = coroutine_study::sync_wait(std::move(t));
    std::println("已处理批次：{} (含因取消停止)", done);
    coroutine_study::check(done > 0 && done < 10, "Part 1/2/4: cancellation stops at a checkpoint and preserves partial progress");

    std::stop_source throwing_src;
    throwing_src.request_stop();
    bool threw_cancelled = false;
    try {
        auto throwing = batch_process_throwing(throwing_src.get_token(), 10);
        (void)coroutine_study::sync_wait(std::move(throwing));
    } catch (const task_cancelled&) {
        threw_cancelled = true;
    }
    coroutine_study::check(threw_cancelled, "Part 3: throwing cancellation path reaches sync_wait caller");

    // ------------------ 进阶任务 ------------------
    // TODO [进阶 1]：用 std::stop_callback 在 request_stop 时记录日志，
    //   观察回调与协程 co_await 点的时序关系。
    //
    // TODO [进阶 2]：让 lazy_task 在嵌套 co_await 时把 stop_token
    //   自动传播到子任务（思考实现思路即可）。
    //
    // TODO [进阶 3]：构造 parent/child 嵌套场景，
    //   parent 取消时 child 在自己的 co_await 点跳出。

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
