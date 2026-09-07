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
namespace coroutine_study_user {

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
        //   2) 把等待工作交给一个有明确 owner 的 worker，future ready 后 h.resume();
        //   3) worker 必须在协程帧销毁前 join；参考实现见 solution.cpp。
        //   骨架占位：同步等待后恢复，安全但看不到跨线程恢复。
        fut_.wait();
        h.resume();
    }

    T await_resume() {
        // TODO [必做]: log 当前线程 id 后，return fut_.get();
        return fut_.get();
    }
};

template <class T>
future_awaiter<T> await_future(std::future<T> f) noexcept {
    return future_awaiter<T>{std::move(f)};
}

} // namespace coroutine_study_user

// 包装函数在用户自己的命名空间里；调用点显式把 std::future 转成 awaitable。
template <class T>
auto await_future(std::future<T> f) noexcept {
    return coroutine_study_user::await_future(std::move(f));
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

    // TODO [必做]: int result = co_await await_future(std::move(fut));
    // 占位实现（同步取值）以便骨架默认可跑：
    int result = co_await await_future(std::move(fut));

    log("coro", "after co_await, result = ", result);
    co_return result * 2;
}

// ─────────────────────────────────────────────────────────────────────
// 进阶任务
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶 A]: 给 future_awaiter 加 std::stop_token 支持，cancel 时走异常路径。
// TODO [进阶 B]: 写 future_awaitable<T> 包装类，并在包装类上提供 operator co_await()。
//   调用点仍保持 co_await await_future(std::move(fut))，把扩展集中在用户包装类型里。
// TODO [进阶 C]: 改用 std::shared_future<T>，思考 get() 的语义差异。

}  // namespace

int main() {
    log("main", "─── A-3：co_await std::future ───");

    auto task = test_future_await();
    int v = coroutine_study::sync_wait(std::move(task));

    log("main", "final result = ", v, " (期望 84)");
    return 0;
}
