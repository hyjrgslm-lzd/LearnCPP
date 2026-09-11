// =============================================================================
// mini/task.hpp —— mini::task<T>，lazy 启动的协程任务
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第三层：promise_type 与 task<T>"
//
// 设计约束（来自 14-mini §"设计约束"）：
//   1. operation_state 等价物 non-movable —— start() 后 promise 与 frame 不能移动；
//   2. completion_signatures 编译期可查询；
//   3. HALO 是否发生须以当前工具链产物验证，不作正确性前提。
//
// 设计选择：
//   - lazy（initial_suspend = suspend_always）；
//   - final_suspend 走 symmetric transfer；机器栈深度是实现观察项；
//   - 禁 copy，move 后原 task 失效（决策 1 方案 B 简化版）；
//   - operation_state 内联存储 —— 不堆分配，HALO 友好（决策 4 方案 A）。
// =============================================================================

#pragma once

#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>
#include <variant>
#include <stdexcept>

namespace mini {

template <typename T = void>
struct task {
    struct promise_type {
        std::variant<std::monostate, T, std::exception_ptr> result_{};
        std::coroutine_handle<> continuation_{};
        bool started_ = false;
        bool consumed_ = false;

        task get_return_object() {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(
                std::coroutine_handle<promise_type> h) noexcept {
                if (auto cont = h.promise().continuation_) return cont;
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }

        template <typename U>
        void return_value(U&& v) requires (!std::is_void_v<T>) {
            result_.template emplace<1>(std::forward<U>(v));
        }
        void unhandled_exception() {
            result_.template emplace<2>(std::current_exception());
        }
    };

    std::coroutine_handle<promise_type> h_{};
    explicit task(std::coroutine_handle<promise_type> h) : h_(h) {}
    task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    task& operator=(task&&) = delete;
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    ~task() { if (h_) h_.destroy(); }

    void start() {
        if (!h_ || h_.done() || std::exchange(h_.promise().started_, true))
            throw std::logic_error("task requires an unstarted frame");
        h_.resume();
    }

    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        if (!h_ || h_.done() || std::exchange(h_.promise().started_, true))
            throw std::logic_error("task can only be awaited before start");
        h_.promise().continuation_ = caller;
        return h_;
    }
    T await_resume() requires (!std::is_void_v<T>) {
        if (!h_ || !h_.done() || std::exchange(h_.promise().consumed_, true))
            throw std::logic_error("task result requires completion and one consumption");
        auto& r = h_.promise().result_;
        if (r.index() == 2) std::rethrow_exception(std::get<2>(r));
        return std::move(std::get<1>(r));
    }

    // TODO[必做]: 让 task 成为 sender —— 定义 completion_signatures + connect()
    //   - completion_signatures = set_value_t(T), set_error_t(exception_ptr), set_stopped_t()
    //   - connect(receiver) -> operation_state，start() 调用 h_.resume()。
};

template <>
struct task<void> {
    struct promise_type {
        std::exception_ptr error_{};
        std::coroutine_handle<> continuation_{};
        bool started_ = false;
        bool consumed_ = false;

        task get_return_object() {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(
                std::coroutine_handle<promise_type> h) noexcept {
                if (auto cont = h.promise().continuation_) return cont;
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }

        void return_void() noexcept {}
        void unhandled_exception() { error_ = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> h_{};
    explicit task(std::coroutine_handle<promise_type> h) : h_(h) {}
    task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~task() { if (h_) h_.destroy(); }

    void start() {
        if (!h_ || h_.done() || std::exchange(h_.promise().started_, true))
            throw std::logic_error("task requires an unstarted frame");
        h_.resume();
    }

    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        if (!h_ || h_.done() || std::exchange(h_.promise().started_, true))
            throw std::logic_error("task can only be awaited before start");
        h_.promise().continuation_ = caller;
        return h_;
    }
    void await_resume() {
        if (!h_ || !h_.done() || std::exchange(h_.promise().consumed_, true))
            throw std::logic_error("task result requires completion and one consumption");
        if (h_.promise().error_) std::rethrow_exception(h_.promise().error_);
    }
};

} // namespace mini
