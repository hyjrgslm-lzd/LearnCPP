#pragma once

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include <stdexec/execution.hpp>

namespace mini {

template <typename T = void>
struct task {
    using sender_concept = stdexec::sender_tag;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(T),
        stdexec::set_error_t(std::exception_ptr),
        stdexec::set_stopped_t()>;

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

    template <typename Receiver>
    struct operation {
        using operation_state_concept = stdexec::operation_state_t;

        task task_;
        Receiver receiver_;
        bool started_{};

        friend void tag_invoke(stdexec::start_t, operation& self) noexcept {
            if (std::exchange(self.started_, true)) return;
            try {
                self.task_.start();
                if (!self.task_.h_.done()) return;
                auto& result = self.task_.h_.promise().result_;
                if (result.index() == 2) {
                    stdexec::set_error(std::move(self.receiver_), std::get<2>(result));
                } else if (result.index() == 1) {
                    stdexec::set_value(std::move(self.receiver_), std::move(std::get<1>(result)));
                } else {
                    stdexec::set_stopped(std::move(self.receiver_));
                }
            } catch (...) {
                stdexec::set_error(std::move(self.receiver_), std::current_exception());
            }
        }
    };

    template <typename Receiver>
    auto connect(Receiver&& receiver) && {
        return operation<std::remove_cvref_t<Receiver>>{
            std::move(*this), std::forward<Receiver>(receiver), false};
    }

    friend auto tag_invoke(stdexec::get_env_t, const task&) noexcept {
        return stdexec::empty_env{};
    }
};

template <>
struct task<void> {
    using sender_concept = stdexec::sender_tag;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(),
        stdexec::set_error_t(std::exception_ptr),
        stdexec::set_stopped_t()>;

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
    void await_resume() {
        if (!h_ || !h_.done() || std::exchange(h_.promise().consumed_, true))
            throw std::logic_error("task result requires completion and one consumption");
        if (h_.promise().error_) std::rethrow_exception(h_.promise().error_);
    }

    template <typename Receiver>
    struct operation {
        using operation_state_concept = stdexec::operation_state_t;

        task task_;
        Receiver receiver_;
        bool started_{};

        friend void tag_invoke(stdexec::start_t, operation& self) noexcept {
            if (std::exchange(self.started_, true)) return;
            try {
                self.task_.start();
                if (!self.task_.h_.done()) return;
                if (self.task_.h_.promise().error_) {
                    stdexec::set_error(std::move(self.receiver_), self.task_.h_.promise().error_);
                } else {
                    stdexec::set_value(std::move(self.receiver_));
                }
            } catch (...) {
                stdexec::set_error(std::move(self.receiver_), std::current_exception());
            }
        }
    };

    template <typename Receiver>
    auto connect(Receiver&& receiver) && {
        return operation<std::remove_cvref_t<Receiver>>{
            std::move(*this), std::forward<Receiver>(receiver), false};
    }

    friend auto tag_invoke(stdexec::get_env_t, const task&) noexcept {
        return stdexec::empty_env{};
    }
};

} // namespace mini
