#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <stop_token>
#include <utility>
#include <variant>

#include "mini/task.hpp"

namespace mini {

template <typename A, typename B>
class when_any_operation {
    task<A> left_;
    task<B> right_;
    std::stop_source stop_;
    std::coroutine_handle<> parent_{};
    std::optional<task<void>> left_runner_;
    std::optional<task<void>> right_runner_;
    std::variant<A, B> winner_{};
    std::exception_ptr first_error_{};
    bool has_winner_{false};

    void publish_left(A value) {
        if (has_winner_) return;
        winner_.template emplace<0>(std::move(value));
        has_winner_ = true;
        stop_.request_stop();
        parent_.resume();
    }

    void publish_right(B value) {
        if (has_winner_) return;
        winner_.template emplace<1>(std::move(value));
        has_winner_ = true;
        stop_.request_stop();
        parent_.resume();
    }

    task<void> run_left() {
        try {
            publish_left(co_await std::move(left_));
        } catch (...) {
            if (!first_error_) first_error_ = std::current_exception();
        }
        co_return;
    }

    task<void> run_right() {
        try {
            publish_right(co_await std::move(right_));
        } catch (...) {
            if (!first_error_) first_error_ = std::current_exception();
        }
        co_return;
    }

public:
    when_any_operation(task<A> left, task<B> right, std::stop_source stop)
        : left_(std::move(left)), right_(std::move(right)), stop_(std::move(stop)) {}

    when_any_operation(when_any_operation&&) = default;
    when_any_operation(const when_any_operation&) = delete;

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> parent) {
        parent_ = parent;
        left_runner_.emplace(run_left());
        right_runner_.emplace(run_right());
        left_runner_->h_.resume();
        right_runner_->h_.resume();
        return true;
    }

    std::variant<A, B> await_resume() {
        if (has_winner_) return std::move(winner_);
        if (first_error_) std::rethrow_exception(first_error_);
        throw std::logic_error("mini::when_any completed without a result");
    }
};

template <typename A, typename B>
auto when_any(task<A> left, task<B> right, std::stop_source stop = {}) {
    return when_any_operation<A, B>{std::move(left), std::move(right), std::move(stop)};
}

} // namespace mini
