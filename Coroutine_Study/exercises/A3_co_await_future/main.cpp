// =====================================================================
// 练习 A-3：co_await 一个 std::future
//   对应文档：02-模块A-三关键字与最小协程.md / 练习 A-3
//   官方参考：
//     - cppreference (awaiter): https://en.cppreference.com/w/cpp/language/coroutines
//     - cppreference (future):  https://en.cppreference.com/w/cpp/thread/future
//     - Raymond Chen 协程系列:  https://devblogs.microsoft.com/oldnewthing/20210504-00/?p=105178
//
// 学习目标：
//   - 亲手实现 awaiter 三方法 (await_ready / await_suspend / await_resume)
//   - 在日志里看到"挂起的线程"和"resume 的线程"是不同的线程
// =====================================================================

#include <coroutine_study/lazy_task.hpp>

#include <chrono>
#include <coroutine>
#include <future>
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
// 必做：future_awaiter<T>
//   - await_ready  : future.wait_for(0s) == ready ?
//   - await_suspend: 起一个线程跑 future.wait()，回来 h.resume()
//   - await_resume : future.get() —— 此时已 ready，不会阻塞
// ─────────────────────────────────────────────────────────────────────
template <class T>
struct future_awaiter {
    std::future<T> fut_;

    bool await_ready() const noexcept {
        // TODO [必做]: return fut_.wait_for(0s) == std::future_status::ready;
        return false;
    }

    void await_suspend(std::coroutine_handle<> h) {
        // TODO [必做]:
        //   1) log 当前线程 id（"await_suspend tid=..."）
        //   2) 启动一个 std::jthread / std::thread，在新线程里 fut_.wait()，然后 h.resume();
        //   3) 注意线程的析构：用 jthread 自动 join 不安全（会死锁），常用做法是 detach
        //      或把 jthread 存到 awaiter 成员里、由 await_resume 之后清理。
        //   下面是一个能编译运行的占位实现：先 fut_.wait() 等就绪再 resume，
        //   这样 await_resume 里的 fut_.get() 不再阻塞——更贴近正确语义。
        //   (this 指向的 awaiter 临时对象存活到 await_resume 返回，捕获安全。)
        std::thread([this, h]() mutable {
            fut_.wait();
            log("await_suspend.thread", "future ready, before resume");
            h.resume();
        }).detach();
    }

    T await_resume() {
        // TODO [必做]: log 当前线程 id 后，return fut_.get();
        return fut_.get();
    }
};

// ADL/成员形式都可以；这里用自由函数作为 operator co_await 适配
template <class T>
future_awaiter<T> operator co_await(std::future<T>&& f) noexcept {
    return future_awaiter<T>{std::move(f)};
}

// ─────────────────────────────────────────────────────────────────────
// 必做：测试协程
// ─────────────────────────────────────────────────────────────────────
lazy_task<int> test_future_await() {
    log("coro", "test_future_await begin");

    auto fut = std::async(std::launch::async, [] {
        log("worker", "sleeping 100ms");
        std::this_thread::sleep_for(100ms);
        return 42;
    });

    // TODO [必做]: int result = co_await std::move(fut);
    // 占位实现（同步取值）以便骨架默认可跑：
    int result = co_await std::move(fut);

    log("coro", "after co_await, result = ", result);
    co_return result * 2;
}

// ─────────────────────────────────────────────────────────────────────
// 进阶任务
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶 A]: 给 future_awaiter 加 std::stop_token 支持，cancel 时走异常路径。
// TODO [进阶 B]: 用模板特化把 operator co_await 直接挂在 std::future<T> 上。
// TODO [进阶 C]: 改用 std::shared_future<T>，思考 get() 的语义差异。

}  // namespace

int main() {
    log("main", "─── A-3：co_await std::future ───");

    auto task = test_future_await();
    int v = task.sync_wait();

    log("main", "final result = ", v, " (期望 84)");
    return 0;
}
