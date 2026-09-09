#pragma once
#include <coroutine>
#include <exception>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace mini {
struct stopped_exception {};

template <typename T = void>
struct task {
    struct promise_type {
        std::variant<std::monostate, T, std::exception_ptr> result_{};
        std::coroutine_handle<> continuation_{};
        task get_return_object() { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        template <typename U>
        void return_value(U&& v) { result_.template emplace<1>(std::forward<U>(v)); }
        void unhandled_exception() { result_.template emplace<2>(std::current_exception()); }
    };
    std::coroutine_handle<promise_type> h_{};
    explicit task(std::coroutine_handle<promise_type> h) : h_(h) {}
    task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    task(const task&) = delete;
    ~task() { if (h_) h_.destroy(); }
    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) { h_.promise().continuation_ = caller; return h_; }
    T await_resume() {
        auto& r = h_.promise().result_;
        if (r.index() == 2) std::rethrow_exception(std::get<2>(r));
        return std::move(std::get<1>(r));
    }
};

template <>
struct task<void> {
    struct promise_type {
        std::exception_ptr error_{};
        task get_return_object() { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { error_ = std::current_exception(); }
    };
    std::coroutine_handle<promise_type> h_{};
    explicit task(std::coroutine_handle<promise_type> h) : h_(h) {}
    task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    task(const task&) = delete;
    ~task() { if (h_) h_.destroy(); }
    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) { return h_; }
    void await_resume() { if (h_.promise().error_) std::rethrow_exception(h_.promise().error_); }
};
} // namespace mini
