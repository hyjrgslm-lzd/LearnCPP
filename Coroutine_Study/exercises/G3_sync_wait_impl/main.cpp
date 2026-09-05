// G-3 实现 sync_wait
// 文档参考：09-模块G-symmetric_transfer与高级task.md 「练习 G-3」
// 官方参考：
//   - cppcoro sync_wait.hpp
//   - Lewis Baker "C++ coroutines: Building a sync_wait"
//   - P2300R10 / [exec.sync.wait]: this_thread::sync_wait 语义
//   - stdexec sync_wait 实现
//
// 目标：实现 sync_wait——在非协程上下文中驱动 task 到完成，把"协程世界"翻译为
//      "同步阻塞世界"。给两个版本：
//        1. 手动循环驱动（简易版，单线程协程链）
//        2. condvar + 内置 receiver 风格（生产版，支持跨线程完成）

#include <condition_variable>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

// ============ 复用 lazy_task<T> ============
template <typename T>
struct lazy_task {
    struct promise_type {
        T result_value{};
        std::exception_ptr result_exception;
        std::coroutine_handle<> continuation;

        lazy_task get_return_object() noexcept {
            return lazy_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<promise_type> h) noexcept
                {
                    auto cont = h.promise().continuation;
                    return cont ? cont : std::noop_coroutine();
                }
                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }
        void return_value(T v) noexcept { result_value = std::move(v); }
        void unhandled_exception() noexcept { result_exception = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> h_;
    explicit lazy_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    lazy_task(const lazy_task&) = delete;
    ~lazy_task() { if (h_) h_.destroy(); }

    bool await_ready() noexcept { return false; }
    auto await_suspend(std::coroutine_handle<> caller) noexcept {
        h_.promise().continuation = caller;
        return h_;
    }
    T await_resume() {
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }

    void resume() { if (h_) h_.resume(); }
    bool done() const noexcept { return h_ && h_.done(); }
};

// ============ 版本 1：手动循环驱动版 sync_wait ============
// 适合：单线程全协程环境（asio io_context 内的协程链）
// 假设：所有 await_suspend 都返回 void 或 coroutine_handle，不旁路 resume
template <typename T>
T sync_wait_simple(lazy_task<T> task)
{
    auto h = task.h_;
    h.resume();                       // 跨过 initial_suspend，进入协程体
    while (!h.done()) h.resume();     // 持续 drain 直到 final_suspend 之后

    auto& p = h.promise();
    if (p.result_exception) std::rethrow_exception(p.result_exception);
    return std::move(p.result_value);
}

// ============ 版本 2：condvar + 工作线程驱动版 sync_wait ============
// 适合：协程可能在另一个线程上完成（io_uring completion / IOCP / 线程池）
// 此版本把 task 的 drain 放到独立 std::thread 上，主线程 condvar wait
template <typename T>
T sync_wait_condvar(lazy_task<T> task)
{
    struct shared_state {
        std::mutex mtx;
        std::condition_variable cv;
        bool done = false;
        std::exception_ptr error;
        std::optional<T> result;
    };
    auto state = std::make_shared<shared_state>();

    // 注：lazy_task 是 move-only。我们用一个轻量 holder 把 handle 直接传给线程
    auto h = task.h_;
    task.h_ = {};   // 把所有权"转移"给 driver；析构 task 时不会 destroy

    std::thread driver([h, state]() {
        try {
            auto handle = h;
            handle.resume();
            while (!handle.done()) handle.resume();

            auto& p = handle.promise();
            if (p.result_exception) {
                state->error = p.result_exception;
            } else {
                state->result = std::move(p.result_value);
            }
            handle.destroy();
        } catch (...) {
            state->error = std::current_exception();
        }
        {
            std::lock_guard lk(state->mtx);
            state->done = true;
        }
        state->cv.notify_one();
    });

    // 主线程阻塞 wait
    {
        std::unique_lock lk(state->mtx);
        state->cv.wait(lk, [&] { return state->done; });
    }
    driver.join();

    if (state->error) std::rethrow_exception(state->error);
    return std::move(*state->result);
}

// ============ 测试 ============
// 一个简易的 "异步" awaitable，仅用于让协程产生若干次挂起
struct yield_once {
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    void await_resume() noexcept {}
};

lazy_task<int> async_compute() {
    co_await yield_once{};
    int a = 10;
    co_await yield_once{};
    int b = 20;
    co_return a + b;
}

lazy_task<int> failing_compute() {
    co_await yield_once{};
    throw std::runtime_error("compute failed");
    co_return 0;   // unreachable
}

int main()
{
    std::printf("===== G-3: sync_wait =====\n\n");

    std::printf("--- 测试 1：sync_wait_simple 成功路径 ---\n");
    {
        int r = sync_wait_simple(async_compute());
        std::printf("  result = %d (expect 30)\n", r);
    }

    std::printf("\n--- 测试 2：sync_wait_simple 错误路径 ---\n");
    {
        try {
            sync_wait_simple(failing_compute());
        } catch (const std::exception& e) {
            std::printf("  caught: %s\n", e.what());
        }
    }

    std::printf("\n--- 测试 3：sync_wait_condvar 成功路径（独立线程驱动）---\n");
    {
        int r = sync_wait_condvar(async_compute());
        std::printf("  result = %d (expect 30)\n", r);
    }

    // TODO [必做]：在笔记中回答：
    //   1. 为什么 main 不能是协程？（从帧分配/启动/销毁三角度）
    //   2. sync_wait 在协程世界 ↔ 同步世界之间承担了什么角色？
    //   3. 简易版 vs condvar 版各自适合什么场景？
    // TODO [进阶]：实现 timeout 版 sync_wait_with_timeout。
    // TODO [进阶]：用 std::atomic_flag 实现 spin-wait 版本。

    std::printf("\n===== Done =====\n");
    return 0;
}
