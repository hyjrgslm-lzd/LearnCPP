#pragma once

#include <coroutine>
#include <exception>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mini {

template <typename T>
class shared_task {
public:
    struct promise_type;

private:
    using handle_t = std::coroutine_handle<promise_type>;

    struct state {
        handle_t producer{};
        bool started{};
        bool done{};
        std::optional<T> value;
        std::exception_ptr error;
        std::vector<std::coroutine_handle<>> waiters;

        ~state() {
            if (producer) producer.destroy();
        }
    };

public:
    struct promise_type {
        state* state_{};

        shared_task get_return_object() {
            auto h = handle_t::from_promise(*this);
            auto s = std::make_shared<state>();
            s->producer = h;
            state_ = s.get();
            return shared_task{std::move(s)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(handle_t h) noexcept {
                auto* s = h.promise().state_;
                s->done = true;
                auto waiters = std::move(s->waiters);
                s->waiters.clear();
                for (auto waiter : waiters) {
                    if (waiter) waiter.resume();
                }
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };

        final_awaiter final_suspend() noexcept { return {}; }

        template <typename U>
        void return_value(U&& value) {
            state_->value.emplace(std::forward<U>(value));
        }

        void unhandled_exception() { state_->error = std::current_exception(); }
    };

    explicit shared_task(std::shared_ptr<state> s) : state_(std::move(s)) {}
    shared_task(const shared_task&) noexcept = default;
    shared_task(shared_task&&) noexcept = default;
    shared_task& operator=(const shared_task&) = default;
    shared_task& operator=(shared_task&&) noexcept = default;

    struct awaiter {
        std::shared_ptr<state> state_ptr;

        bool await_ready() const noexcept { return state_ptr->done; }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
            if (state_ptr->done) return caller;
            state_ptr->waiters.push_back(caller);
            if (!state_ptr->started) {
                state_ptr->started = true;
                return state_ptr->producer;
            }
            return std::noop_coroutine();
        }

        T await_resume() {
            if (state_ptr->error) std::rethrow_exception(state_ptr->error);
            return *state_ptr->value;
        }
    };

    awaiter operator co_await() const { return awaiter{state_}; }

private:
    std::shared_ptr<state> state_;
};

} // namespace mini
