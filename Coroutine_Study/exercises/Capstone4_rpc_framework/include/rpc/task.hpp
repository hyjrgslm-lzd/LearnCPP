// =============================================================================
// rpc/task.hpp —— 最小 task<T>（带 symmetric transfer）
//
// 对应文档：13-第三阶段结课-RPC框架.md（Client/Server handler 的协程载体）
//
// 设计要点：
//   - lazy（initial_suspend = suspend_always）；
//   - final_suspend 走 symmetric transfer，避免深嵌套栈溢出；
//   - 不允许 copy / move-after-start；
//   - 不暴露 coroutine_handle 给跨模块边界。
// =============================================================================

#pragma once

#include <coroutine>
#include <exception>
#include <utility>
#include <variant>

namespace rpc {

template <typename T = void>
struct task {
    struct promise_type {
        std::variant<std::monostate, T, std::exception_ptr> result_{};
        std::coroutine_handle<> continuation_{};

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

    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        h_.promise().continuation_ = caller;
        return h_;
    }
    T await_resume() requires (!std::is_void_v<T>) {
        auto& r = h_.promise().result_;
        if (r.index() == 2) std::rethrow_exception(std::get<2>(r));
        return std::move(std::get<1>(r));
    }

    // TODO[必做]: 让 task 也实现 sender concept（H-1 桥接）
    // 见 14-第三阶段结课-mini协程库实现.md 的设计参考。
};

template <>
struct task<void> {
    struct promise_type {
        std::exception_ptr error_{};
        std::coroutine_handle<> continuation_{};

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
    task& operator=(task&&) = delete;
    ~task() { if (h_) h_.destroy(); }

    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        h_.promise().continuation_ = caller;
        return h_;
    }
    void await_resume() {
        if (h_.promise().error_) std::rethrow_exception(h_.promise().error_);
    }
};

} // namespace rpc
