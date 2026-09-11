// =====================================================================
// 练习 D-3：final_suspend 中的 symmetric transfer
// 文档参考：C09_Coroutines/06-模块D-promise_type全解.md  -> 练习 D-3
// 官方参考：
//   - Lewis Baker, "C++ coroutines: Symmetric transfer"
//   - P0913R1: Symmetric coroutine control transfer
//   - cppreference: std::noop_coroutine
//   - cppcoro task.hpp 的 final_suspend
// 学习要点：
//   1. await_suspend 返回 coroutine_handle<> 即 symmetric transfer，
//      框架直接跳到目标，跳过中间栈帧。
//   2. 没有 symmetric transfer 时，N 层嵌套 co_await 会随 N 线性增长栈深。
//   3. continuation 为空时返回 std::noop_coroutine() 而不是 {}.
// =====================================================================
#include <coroutine>
#include <exception>
#include <iostream>
#include <print>
#include <stdexcept>
#include <utility>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

// ---------------------------------------------------------------------
// 两个版本的 task：
//   - lazy_task_symmetric    : final_suspend 走 symmetric transfer
//   - lazy_task_resume_chain : final_suspend 直接 .resume() 等待者（栈危险）
// 通过宏 USE_SYMMETRIC 控制 main 中 chain 实验使用哪一版。
// ---------------------------------------------------------------------

template <typename T>
struct lazy_task_symmetric {
    struct promise_type {
        T                       result_value{};
        std::exception_ptr      result_exception{};
        std::coroutine_handle<> continuation{};

        lazy_task_symmetric get_return_object() {
            return lazy_task_symmetric{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            // TODO [必做 1]：当 continuation 存在时返回它，否则返回 noop_coroutine
            //   —— 这是 symmetric transfer 的核心。
            std::coroutine_handle<>
            await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                (void)h;
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }

        void return_value(T v) { result_value = std::move(v); }
        void unhandled_exception() noexcept {
            result_exception = std::current_exception();
        }
    };

    std::coroutine_handle<promise_type> h_{};
    explicit lazy_task_symmetric(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    lazy_task_symmetric(const lazy_task_symmetric&) = delete;
    lazy_task_symmetric(lazy_task_symmetric&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~lazy_task_symmetric() { if (h_) h_.destroy(); }

    T get() {
        if (!h_) throw std::logic_error("empty");
        h_.resume();
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }

    // operator co_await（symmetric transfer 版）
    bool await_ready() const noexcept { return !h_ || h_.done(); }
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> caller) noexcept {
        h_.promise().continuation = caller;
        return h_;
    }
    T await_resume() {
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }
};

// ---------------------------------------------------------------------
// 反例：final_suspend 直接 .resume() 等待者；嵌套深时容易栈溢出。
// ---------------------------------------------------------------------
template <typename T>
struct lazy_task_resume_chain {
    struct promise_type {
        T                       result_value{};
        std::exception_ptr      result_exception{};
        std::coroutine_handle<> continuation{};

        lazy_task_resume_chain get_return_object() {
            return lazy_task_resume_chain{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            // TODO [必做 2]：返回 void，并在内部直接 .resume() 等待者。
            //   这就是"在 A 的栈帧里 resume B"的反例。
            void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                if (h.promise().continuation)
                    h.promise().continuation.resume();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }

        void return_value(T v) { result_value = std::move(v); }
        void unhandled_exception() noexcept {
            result_exception = std::current_exception();
        }
    };

    std::coroutine_handle<promise_type> h_{};
    explicit lazy_task_resume_chain(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    lazy_task_resume_chain(const lazy_task_resume_chain&) = delete;
    lazy_task_resume_chain(lazy_task_resume_chain&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~lazy_task_resume_chain() { if (h_) h_.destroy(); }

    T get() {
        if (!h_) throw std::logic_error("empty");
        h_.resume();
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }
    bool await_ready() const noexcept { return !h_ || h_.done(); }
    void await_suspend(std::coroutine_handle<> caller) noexcept {
        h_.promise().continuation = caller;
        h_.resume();
    }
    T await_resume() {
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }
};

// ---------------------------------------------------------------------
// N 层嵌套链：用 symmetric transfer 版应该能轻松跑通 1000 层。
// ---------------------------------------------------------------------
lazy_task_symmetric<int> chain_sym(int n) {
    if (n == 0) co_return 0;
    int v = co_await chain_sym(n - 1);
    co_return v + 1;
}

lazy_task_resume_chain<int> chain_naive(int n) {
    if (n == 0) co_return 0;
    int v = co_await chain_naive(n - 1);
    co_return v + 1;
}

int main() try {
    std::println("===== 练习 D-3：final_suspend symmetric transfer =====\n");

    // ------------------ 浅测试 ------------------
    {
        auto t = chain_sym(10);
        int r = t.get();
        std::println("[symmetric] chain_sym(10) = {} (期望 10)", r);
        check(r == 10, "TODO: final_suspend must return continuation when present");
    }

    // ------------------ 深嵌套 ------------------
    {
        // TODO [必做 3]：用 symmetric 版跑 1000 层应不爆栈。
        auto t = chain_sym(1000);
        int r = t.get();
        std::println("[symmetric] chain_sym(1000) = {} (期望 1000)", r);
        check(r == 1000, "symmetric transfer chain should reach the outer caller");
    }

    // ------------------ 反例（小心栈）------------------
    {
        // TODO [必做 4]：先用 N=10 验证语义，再尝试 N=1000 观察是否栈溢出。
        //   不同平台/编译器结果不一样；MSVC Debug 默认 1MB 栈通常 ~100 层就崩。
        auto t = chain_naive(10);
        int r = t.get();
        std::println("[resume-chain] chain_naive(10) = {} (期望 10)", r);
        check(r == 10, "direct-resume comparison should still preserve shallow semantics");
    }

    // ------------------ 进阶 ------------------
    // TODO [进阶 1]：用 -fsanitize=address + 显式栈深计数对比两版栈使用。
    // TODO [进阶 2]：研究 std::noop_coroutine 的语义与适用场景。
    // TODO [进阶 3]：在 task 的 await_suspend 中也使用 symmetric transfer，
    //   把 caller -> task -> task -> ... 的链条全部走 symmetric。

    std::println("\n===== Done =====");
    return 0;
} catch (const std::exception& e) {
    std::cerr << "starter check failed: " << e.what() << '\n';
    return 1;
}
