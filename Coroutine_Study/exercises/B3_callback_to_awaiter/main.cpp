// =====================================================================
// 练习 B-3：把回调 API 包成 awaiter
//   对应文档：03-模块B-generator与task的使用.md / 练习 B-3
//   官方参考：
//     - Raymond Chen: https://devblogs.microsoft.com/oldnewthing/20210507-00/?p=105212
//     - Asio awaitable: https://www.boost.org/doc/libs/master/doc/html/boost_asio/overview/composition/cpp20_coroutines.html
//
// 学习目标：
//   - 看到 awaiter 的固定模式：保存 handle / 启动异步 / 回调里 resume
//   - 看到协程恢复后的代码可能跑在异步回调线程上
// =====================================================================

#include <coroutine_study/lazy_task.hpp>

#include <chrono>
#include <coroutine>
#include <iostream>
#include <syncstream>
#include <thread>

using namespace std::chrono_literals;
using coroutine_study::lazy_task;

namespace {

template <class... Args>
void log(const char* tag, Args&&... args) {
    std::osyncstream os{std::cout};
    os << "[" << tag << "] ";
    ((os << args), ...);
    os << "  (tid=" << std::this_thread::get_id() << ")\n";
}

// ─────────────────────────────────────────────────────────────────────
// 必做 1：模拟一个回调式异步 API
//   把 (a + b) 在新线程 sleep 100ms 后通过 cb(result) 通知调用方
// ─────────────────────────────────────────────────────────────────────
template <class Callback>
void async_add(int a, int b, Callback&& cb) {
    log("async_add", "compute ", a, " + ", b);
    std::this_thread::sleep_for(100ms);
    cb(a + b);
}

// ─────────────────────────────────────────────────────────────────────
// 必做 2：AsyncAddAwaiter
// ─────────────────────────────────────────────────────────────────────
struct AsyncAddAwaiter {
    int a_;
    int b_;
    int result_{};

    bool await_ready() const noexcept {
        // TODO [必做 2.a]: 异步操作还没启动，永远 false
        return false;
    }

    void await_suspend(std::coroutine_handle<> h) {
        // TODO [必做 2.b]:
        //   log("await_suspend", "tid=...");
        //   async_add(a_, b_, [this, h](int r) {
        //       result_ = r;          // 先写 result
        //       h.resume();           // 后 resume；真实跨线程版本还要保证生命周期与同步
        //   });
        log("await_suspend", "schedule async_add(", a_, ",", b_, ")");
        async_add(a_, b_, [this, h](int r) mutable {
            result_ = r;
            h.resume();
        });
    }

    int await_resume() noexcept {
        // TODO [必做 2.c]: log 当前线程 id，return result_;
        return result_;
    }
};

// ─────────────────────────────────────────────────────────────────────
// 必做 3：使用上面的 awaiter 的协程
// ─────────────────────────────────────────────────────────────────────
lazy_task<int> compute_with_callback(int x, int y) {
    log("coro", "before co_await");

    // TODO [必做 3]: int sum = co_await AsyncAddAwaiter{x, y};
    AsyncAddAwaiter awaiter{x, y};
    int sum = co_await awaiter;

    log("coro", "after co_await, sum=", sum);
    co_return sum * 3;
}

// ─────────────────────────────────────────────────────────────────────
// 进阶任务
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶 A]: 给 awaiter 加错误路径——20% 概率回调 error_code，await_resume 中 throw。
// TODO [进阶 B]: 写通用 callback_awaiter<AsyncFunc, Args...>，自动适配各类回调签名。
// TODO [进阶 C]: 替换为真实异步 API（如 Asio async_read），把 completion handler 接到 awaiter。
// TODO [进阶 D]: 加 std::stop_token 支持，cancel 时尝试取消底层异步操作。

}  // namespace

int main() {
    log("main", "─── B-3：把回调 API 包成 awaiter ───");

    auto t = compute_with_callback(7, 5);
    int v = coroutine_study::sync_wait(std::move(t));

    log("main", "final = ", v, "  (期望 (7+5)*3 = 36)");
    return 0;
}
