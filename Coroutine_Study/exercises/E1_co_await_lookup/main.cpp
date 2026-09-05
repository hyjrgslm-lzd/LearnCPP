// =====================================================================
// 练习 E-1：co_await 三步查找
// 文档参考：Coroutine_Study/07-模块E-awaitable三层与co_await变换.md  -> 练习 E-1
// 官方参考：
//   - Lewis Baker, "C++ coroutines: Understanding operator co_await"
//   - Raymond Chen, "The many meanings of co_await"
//   - N4775 7.6.2.3 (co_await spec)
//   - C++20 [expr.await] / [coroutine.trivial.awaitables]
// 学习要点：
//   优先级（从高到低）：
//     1) promise.await_transform(expr)
//     2) expr.operator co_await()  (成员)
//     3) ADL operator co_await(expr)
//   await_transform 一旦定义且可调用，会"覆盖" expr 自身的 awaitable 身份。
// =====================================================================
#include <coroutine>
#include <exception>
#include <iostream>
#include <print>
#include <utility>

// ---------------------------------------------------------------------
// awaitable_A —— 期望被 await_transform 拦截
// ---------------------------------------------------------------------
struct awaitable_A {
    bool await_ready() noexcept {
        std::println("    awaitable_A::await_ready (should NOT be called when intercepted)");
        return false;
    }
    void await_suspend(std::coroutine_handle<> h) noexcept {
        std::println("    awaitable_A::await_suspend (should NOT be called when intercepted)");
        h.resume();
    }
    int await_resume() noexcept {
        std::println("    awaitable_A::await_resume (should NOT be called when intercepted)");
        return 1;
    }
};

// ---------------------------------------------------------------------
// awaitable_B —— 自带成员 operator co_await
// ---------------------------------------------------------------------
struct awaitable_B {
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept { h.resume(); }
    int  await_resume() noexcept { return 2; }

    // TODO [必做 1]：成员 operator co_await 返回一个 wrapper awaitable，
    //   await_resume 返回 20 以便区分。
    auto operator co_await() noexcept {
        struct wrapper {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) noexcept {
                std::println("    [B member] wrapper::await_suspend");
                h.resume();
            }
            int await_resume() noexcept { return 20; }
        };
        return wrapper{};
    }
};

// ---------------------------------------------------------------------
// awaitable_C —— 仅靠 ADL 命名空间内的 operator co_await
// ---------------------------------------------------------------------
namespace custom_ns {
    struct awaitable_C {
        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) noexcept { h.resume(); }
        int  await_resume() noexcept { return 3; }
    };

    // TODO [必做 2]：ADL operator co_await，返回 wrapper awaitable，
    //   await_resume 返回 30。
    inline auto operator co_await(awaitable_C) noexcept {
        struct wrapper {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) noexcept {
                std::println("    [C ADL] wrapper::await_suspend");
                h.resume();
            }
            int await_resume() noexcept { return 30; }
        };
        return wrapper{};
    }
}

// ---------------------------------------------------------------------
// transform_task：promise 含 await_transform，对 A / B 进行拦截
// ---------------------------------------------------------------------
struct transform_task {
    struct promise_type {
        transform_task get_return_object() {
            return transform_task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }

        // TODO [必做 3]：拦截 awaitable_A，返回一个 await_resume == 100 的 awaitable
        auto await_transform(awaitable_A) noexcept {
            struct intercepted {
                bool await_ready() noexcept { return false; }
                void await_suspend(std::coroutine_handle<> h) noexcept {
                    std::println("    [await_transform] intercepted A");
                    h.resume();
                }
                int await_resume() noexcept { return 100; }
            };
            return intercepted{};
        }

        // TODO [必做 4]：拦截 awaitable_B，返回 await_resume == 200 的 awaitable，
        //   验证即使 B 自身有成员 operator co_await，await_transform 优先生效。
        auto await_transform(awaitable_B) noexcept {
            struct intercepted {
                bool await_ready() noexcept { return false; }
                void await_suspend(std::coroutine_handle<> h) noexcept {
                    std::println("    [await_transform] intercepted B (overrides member)");
                    h.resume();
                }
                int await_resume() noexcept { return 200; }
            };
            return intercepted{};
        }

        // 不拦截 awaitable_C —— 通过通用模板兜底，原样转发后再走 ADL operator co_await
        template <class A>
        decltype(auto) await_transform(A&& a) noexcept {
            return std::forward<A>(a);
        }
    };

    std::coroutine_handle<promise_type> h_;
    explicit transform_task(std::coroutine_handle<promise_type> h) : h_(h) {}
    transform_task(const transform_task&) = delete;
    transform_task(transform_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~transform_task() { if (h_) h_.destroy(); }
    void run() { h_.resume(); }
};

// ---------------------------------------------------------------------
// plain_task：promise 不含 await_transform —— 用于对照实验
// ---------------------------------------------------------------------
struct plain_task {
    struct promise_type {
        plain_task get_return_object() {
            return plain_task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }
    };
    std::coroutine_handle<promise_type> h_;
    explicit plain_task(std::coroutine_handle<promise_type> h) : h_(h) {}
    plain_task(const plain_task&) = delete;
    plain_task(plain_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~plain_task() { if (h_) h_.destroy(); }
    void run() { h_.resume(); }
};

// ---------------------------------------------------------------------
// 协程体：分别 co_await 三种类型
// ---------------------------------------------------------------------
transform_task demo_with_transform() {
    std::println("  [transform_task] co_await awaitable_A:");
    int a = co_await awaitable_A{};
    std::println("    => result = {} (期望 100)\n", a);

    std::println("  [transform_task] co_await awaitable_B:");
    int b = co_await awaitable_B{};
    std::println("    => result = {} (期望 200，await_transform 覆盖了成员 operator co_await)\n", b);

    std::println("  [transform_task] co_await custom_ns::awaitable_C:");
    int c = co_await custom_ns::awaitable_C{};
    std::println("    => result = {} (期望 30，走 ADL operator co_await)\n", c);

    co_return;
}

plain_task demo_no_transform() {
    std::println("  [plain_task] co_await awaitable_B:");
    int b = co_await awaitable_B{};
    std::println("    => result = {} (期望 20，走成员 operator co_await)\n", b);

    std::println("  [plain_task] co_await custom_ns::awaitable_C:");
    int c = co_await custom_ns::awaitable_C{};
    std::println("    => result = {} (期望 30，走 ADL)\n", c);

    co_return;
}

int main() {
    std::println("===== 练习 E-1：co_await 三步查找 =====\n");

    std::println("--- 实验 1：promise 含 await_transform ---");
    {
        auto t = demo_with_transform();
        t.run();
    }

    std::println("--- 实验 2：promise 不含 await_transform（对照）---");
    {
        auto t = demo_no_transform();
        t.run();
    }

    // ------------------ 进阶 ------------------
    // TODO [进阶 1]：写一个 logging_promise，await_transform 是泛型 (template T) 模板兜底，
    //   记录所有 co_await 表达式后再"原样转发"。
    // TODO [进阶 2]：让一个类型同时拥有成员 operator co_await 和 ADL operator co_await，
    //   观察 plain_task 中编译器会选哪一个（标准规定成员优先）。
    // TODO [进阶 3]：在 await_transform 中包装 wrapper 实现"对所有 co_await 点的计时"。

    std::println("===== Done =====");
    return 0;
}
