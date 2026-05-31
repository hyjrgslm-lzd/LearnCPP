// =====================================================================
// 练习 E-2：await_suspend 的三种返回值
// 文档参考：Coroutine_Study/07-模块E-awaitable三层与co_await变换.md  -> 练习 E-2
// 官方参考：
//   - Lewis Baker, "C++ coroutines: Symmetric transfer"
//   - Raymond Chen, "The await_suspend return type"
//   - P0913R1: symmetric coroutine control transfer
// 学习要点：
//   void  -> 无条件挂起，外部代码负责 resume。
//   bool  -> true=挂起；false=不挂起，立即继续。
//   handle-> symmetric transfer，框架直接跳到目标，跳过中间栈帧。
// =====================================================================
#include <coroutine>
#include <exception>
#include <iostream>
#include <print>
#include <utility>

// ---------------------------------------------------------------------
// 极简 task：仅用于驱动协程
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
    auto handle() const noexcept { return h_; }
    void start() { h_.resume(); }
};

// ---------------------------------------------------------------------
// awaiter_void：返回 void —— 必须由"外部代码"手动 resume
// ---------------------------------------------------------------------
struct awaiter_void {
    bool await_ready() noexcept {
        std::println("    [void] await_ready -> false");
        return false;
    }
    // TODO [必做 1]：保存 handle，让外部稍后 resume；返回 void。
    void await_suspend(std::coroutine_handle<> h) noexcept {
        std::println("    [void] await_suspend -> 挂起，等外部 resume");
        saved_handle = h;
    }
    void await_resume() noexcept {
        std::println("    [void] await_resume");
    }
    static inline std::coroutine_handle<> saved_handle{};
};

// ---------------------------------------------------------------------
// awaiter_bool_true：返回 bool=true（同 void 行为）
// ---------------------------------------------------------------------
struct awaiter_bool_true {
    bool await_ready() noexcept {
        std::println("    [bool=true] await_ready -> false");
        return false;
    }
    // TODO [必做 2]：返回 true 表示挂起；保存 handle 供外部 resume。
    bool await_suspend(std::coroutine_handle<> h) noexcept {
        std::println("    [bool=true] await_suspend -> true (挂起)");
        saved_handle = h;
        return true;
    }
    void await_resume() noexcept {
        std::println("    [bool=true] await_resume");
    }
    static inline std::coroutine_handle<> saved_handle{};
};

// ---------------------------------------------------------------------
// awaiter_bool_false：返回 bool=false（不挂起，立即继续）
// ---------------------------------------------------------------------
struct awaiter_bool_false {
    bool await_ready() noexcept {
        std::println("    [bool=false] await_ready -> false (但稍后 await_suspend 会反悔)");
        return false;
    }
    // TODO [必做 3]：返回 false —— 表示"算了不挂起"，await_resume 会立即被调用。
    bool await_suspend(std::coroutine_handle<>) noexcept {
        std::println("    [bool=false] await_suspend -> false (不挂起)");
        return false;
    }
    void await_resume() noexcept {
        std::println("    [bool=false] await_resume (立即被调用)");
    }
};

// ---------------------------------------------------------------------
// awaiter_symmetric：返回 coroutine_handle<> —— symmetric transfer
// ---------------------------------------------------------------------
struct awaiter_symmetric {
    std::coroutine_handle<> target;

    bool await_ready() noexcept {
        std::println("    [symmetric] await_ready -> false");
        return false;
    }
    // TODO [必做 4]：返回目标 handle，框架直接跳到 target，
    //   不在当前协程的栈帧里 .resume() target。
    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
        std::println("    [symmetric] await_suspend -> 直接跳到 target");
        return target;
    }
    void await_resume() noexcept {
        std::println("    [symmetric] await_resume");
    }
};

// ---------------------------------------------------------------------
// 实验协程
// ---------------------------------------------------------------------
simple_task coro_void() {
    std::println("  [coro_void] before co_await");
    co_await awaiter_void{};
    std::println("  [coro_void] after co_await");
    co_return;
}

simple_task coro_bool_true() {
    std::println("  [coro_bool_true] before co_await");
    co_await awaiter_bool_true{};
    std::println("  [coro_bool_true] after co_await");
    co_return;
}

simple_task coro_bool_false() {
    std::println("  [coro_bool_false] before co_await");
    co_await awaiter_bool_false{};
    std::println("  [coro_bool_false] after co_await");
    co_return;
}

simple_task coro_B() {
    std::println("  [coro_B] running");
    co_return;
}

simple_task coro_A(std::coroutine_handle<> b_handle) {
    std::println("  [coro_A] before symmetric transfer");
    co_await awaiter_symmetric{b_handle};
    std::println("  [coro_A] after symmetric transfer (在 B 完成后才打印)");
    co_return;
}

int main() {
    std::println("===== 练习 E-2：await_suspend 三种返回值 =====\n");

    // ------------------ void ------------------
    std::println("--- 实验 1：返回 void ---");
    {
        auto t = coro_void();
        t.start();   // 挂起在 awaiter_void::await_suspend
        std::println("  ... [main] 控制权回到这里。手动 resume：");
        awaiter_void::saved_handle.resume();
    }
    std::println("");

    // ------------------ bool=true ------------------
    std::println("--- 实验 2：返回 bool=true ---");
    {
        auto t = coro_bool_true();
        t.start();
        std::println("  ... [main] 控制权回到这里。手动 resume：");
        awaiter_bool_true::saved_handle.resume();
    }
    std::println("");

    // ------------------ bool=false ------------------
    std::println("--- 实验 3：返回 bool=false ---");
    {
        auto t = coro_bool_false();
        t.start(); // 不挂起，应该一气跑完
    }
    std::println("");

    // ------------------ symmetric transfer ------------------
    std::println("--- 实验 4：返回 coroutine_handle (symmetric transfer) ---");
    {
        auto b = coro_B();
        auto a = coro_A(b.handle());
        a.start();
        // TODO [必做 5]：分析输出顺序，观察 A 如何"先把控制权交给 B"
        //   再在 B 完成后回来。
    }

    // ------------------ 进阶 ------------------
    // TODO [进阶 1]：构造 1000 层 void 链 vs symmetric 链，测量栈深差异。
    // TODO [进阶 2]：用 bool=false 实现"轮询命中"awaiter（命中即不挂起）。
    // TODO [进阶 3]：研究 std::suspend_always / suspend_never 的标准库实现，
    //   它们的 await_suspend 返回什么类型？

    std::println("\n===== Done =====");
    return 0;
}
