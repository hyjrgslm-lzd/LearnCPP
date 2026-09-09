// =====================================================================
// 练习 D-1：从零写 lazy_task<T> —— promise_type 8 hook 全实现
// 文档参考：C09_Coroutines/06-模块D-promise_type全解.md  -> 练习 D-1
// 官方参考：
//   - Lewis Baker, "C++ coroutines: Understanding the promise type"
//   - Lewis Baker, "C++ coroutines: Managing the coroutine frame"
//   - cppcoro task.hpp
//   - P0913R1: Symmetric coroutine control transfer
// 学习要点：
//   1. 8 个可定制点的调用时机。
//   2. final_suspend 必须挂起，否则协程帧在被消费前已销毁。
//   3. 本练习让 unhandled_exception 存储 exception_ptr，再由消费者重抛。
//
// 注意：本练习从 0 开始，不允许 include 顶层提供的 lazy_task.hpp。
//       请在每个 hook 函数体内填写 TODO，类型层骨架已给出。
// =====================================================================
#include <coroutine>
#include <cstddef>
#include <exception>
#include <iostream>
#include <new>
#include <print>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename T>
struct lazy_task {
    struct promise_type {
        // 以下成员存放协程结果与等待者
        // 提示：使用 union/optional/aligned_storage 可避免 T 的默认构造要求。
        //       为简化，本练习允许假定 T 是默认可构造的；进阶时再优化。
        T                        result_value{};
        std::exception_ptr       result_exception{};
        std::coroutine_handle<>  continuation{};

        // ---- 1. get_return_object ----
        lazy_task get_return_object() {
            // TODO [必做 1]：返回 lazy_task{ from_promise(*this) }
            return lazy_task{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        // ---- 2. initial_suspend ----
        std::suspend_always initial_suspend() noexcept {
            // TODO [必做 2]：lazy 语义返回 suspend_always{}（默认即可）。
            return {};
        }

        // ---- 3. final_suspend ----
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<>
            await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                // TODO [必做 3]：若 continuation 非空，返回它（symmetric transfer）；
                //   否则返回 std::noop_coroutine()。
                if (h.promise().continuation)
                    return h.promise().continuation;
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept {
            // TODO [必做 4]：必须挂起，不能 suspend_never。
            return {};
        }

        // ---- 4. return_value ----
        void return_value(T value) {
            // TODO [必做 5]：保存 std::move(value) 到 result_value。
            result_value = std::move(value);
        }

        // ---- 5. unhandled_exception ----
        void unhandled_exception() noexcept {
            // TODO [必做 6]：把 std::current_exception() 存入 result_exception。
            //   本 task 选择 noexcept + 存储异常；直接 throw 会改变驱动端的异常传播契约。
            result_exception = std::current_exception();
        }

        // ---- 6. yield_value (本 task 不用 generator，可省略) ----
        // 留空。

        // ---- 7. operator new / operator delete ----
        static void* operator new(std::size_t size) {
            // TODO [必做 7]：可以直接 return ::operator new(size);
            //   学习目的：理解协程帧的分配点。
            return ::operator new(size);
        }
        static void operator delete(void* ptr, std::size_t /*size*/) noexcept {
            ::operator delete(ptr);
        }

        // ---- 8. get_return_object_on_allocation_failure ----
        static lazy_task get_return_object_on_allocation_failure() {
            // TODO [必做 8]：通常 throw std::bad_alloc{} 或返回空 lazy_task。
            throw std::bad_alloc{};
        }
    };

    // ---- task 自身 ----
    std::coroutine_handle<promise_type> h_{};

    explicit lazy_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    lazy_task(const lazy_task&) = delete;
    lazy_task& operator=(const lazy_task&) = delete;
    lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    lazy_task& operator=(lazy_task&& o) noexcept {
        if (this != &o) {
            if (h_) h_.destroy();
            h_ = std::exchange(o.h_, {});
        }
        return *this;
    }
    ~lazy_task() { if (h_) h_.destroy(); }

    // 阻塞驱动：直接 resume 直到 final_suspend，然后取结果。
    T get() {
        if (!h_) throw std::logic_error("empty task");
        h_.resume();
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }

    // 让 task 可被 co_await
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
// 测试协程：正常返回
// ---------------------------------------------------------------------
lazy_task<int> compute() {
    int x = 10;
    int y = 20;
    co_return x + y;
}

// ---------------------------------------------------------------------
// 测试协程：抛异常
// ---------------------------------------------------------------------
lazy_task<int> faulty() {
    if (true) throw std::runtime_error("boom");
    co_return 0;
}

// ---------------------------------------------------------------------
// 测试协程：嵌套 co_await
// ---------------------------------------------------------------------
lazy_task<int> inner() { co_return 42; }
lazy_task<int> outer() {
    int v = co_await inner();
    co_return v * 2;
}

int main() {
    std::println("===== 练习 D-1：从零写 lazy_task<T> =====\n");

    // ------------------ 正常流程 ------------------
    {
        auto t = compute();
        int r = t.get();
        std::println("[normal] compute().get() = {} (期望 30)", r);
    }

    // ------------------ 异常流程 ------------------
    {
        auto t = faulty();
        try {
            (void)t.get();
            std::println("[error] 未捕获到异常 —— 检查 unhandled_exception 实现");
        } catch (const std::runtime_error& e) {
            std::println("[error] 捕获到 \"{}\" —— 期望 \"boom\"", e.what());
        }
    }

    // ------------------ 嵌套 co_await ------------------
    {
        auto t = outer();
        int r = t.get();
        std::println("[nested] outer().get() = {} (期望 84)", r);
    }

    // ------------------ 进阶 ------------------
    // TODO [进阶 1]：用 -fdump-tree-coro / /d1reportSingleClassLayout
    //   打印 lazy_task<int> 的协程帧布局。
    // TODO [进阶 2]：把 result_value 改为 std::optional<T>，
    //   去除"T 必须默认可构造"的限制。
    // TODO [进阶 3]：在多线程下验证 coroutine_handle::resume() 的线程安全性。

    std::println("\n===== Done =====");
    return 0;
}
