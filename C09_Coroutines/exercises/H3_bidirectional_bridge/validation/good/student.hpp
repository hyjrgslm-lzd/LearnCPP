#pragma once

#include <stdexec/execution.hpp>

#include <atomic>
#include <coroutine>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace ex = stdexec;

struct stopped_tag {};

struct bridge_state {
    static constexpr std::uintptr_t completed = 1;

    std::atomic<std::uintptr_t> continuation{0};
    std::variant<std::monostate, int, std::exception_ptr, stopped_tag> result;

    void complete(std::coroutine_handle<> h) noexcept
    {
        const auto old = continuation.exchange(completed, std::memory_order_acq_rel);
        if (old != 0) {
            h.resume();
        }
    }
};

template <typename Promise>
struct bridge_receiver {
    using receiver_concept = ex::receiver_t;

    std::coroutine_handle<Promise> coroutine;
    bridge_state* state;

    template <typename V>
    friend void tag_invoke(ex::set_value_t, bridge_receiver&& self, V&& value) noexcept
    {
        self.state->result.template emplace<int>(static_cast<int>(std::forward<V>(value)));
        self.state->complete(self.coroutine);
    }

    template <typename E>
    friend void tag_invoke(ex::set_error_t, bridge_receiver&& self, E&& error) noexcept
    {
        if constexpr (std::is_same_v<std::decay_t<E>, std::exception_ptr>) {
            self.state->result.template emplace<std::exception_ptr>(std::forward<E>(error));
        } else {
            try {
                throw std::forward<E>(error);
            } catch (...) {
                self.state->result.template emplace<std::exception_ptr>(std::current_exception());
            }
        }
        self.state->complete(self.coroutine);
    }

    friend void tag_invoke(ex::set_stopped_t, bridge_receiver&& self) noexcept
    {
        self.state->result.template emplace<stopped_tag>();
        self.state->complete(self.coroutine);
    }

    friend auto tag_invoke(ex::get_env_t, const bridge_receiver&) noexcept
    {
        return ex::empty_env{};
    }
};

template <class Sender>
class sender_awaitable {
public:
    explicit sender_awaitable(Sender s) : sender(std::move(s)) {}

    bool await_ready() const noexcept { return false; }

    template <typename Promise>
    bool await_suspend(std::coroutine_handle<Promise> h)
    {
        using receiver_t = bridge_receiver<Promise>;
        auto op = ex::connect(std::move(sender), receiver_t{h, &state});
        op_ = std::make_unique<op_holder<decltype(op)>>(std::move(op));
        op_->start();
        const auto old = state.continuation.exchange(
            reinterpret_cast<std::uintptr_t>(h.address()),
            std::memory_order_acq_rel);
        return old != bridge_state::completed;
    }

    int await_resume()
    {
        if (auto* value = std::get_if<int>(&state.result)) return *value;
        if (auto* error = std::get_if<std::exception_ptr>(&state.result)) std::rethrow_exception(*error);
        throw std::runtime_error("sender did not produce a value");
    }

private:
    struct op_base {
        virtual ~op_base() = default;
        virtual void start() noexcept = 0;
    };

    template <class Op>
    struct op_holder final : op_base {
        explicit op_holder(Op op) : op_(std::move(op)) {}
        void start() noexcept override { ex::start(op_); }
        Op op_;
    };

    Sender sender;
    bridge_state state;
    std::unique_ptr<op_base> op_;
};

template <class T>
class my_task {
public:
    using sender_concept = ex::sender_tag;

    struct promise_type;
    using handle_t = std::coroutine_handle<promise_type>;

    struct promise_type {
        T value{};
        std::exception_ptr error;
        std::coroutine_handle<> continuation;

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(handle_t h) noexcept
            {
                auto cont = h.promise().continuation;
                return cont ? cont : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };

        my_task get_return_object() noexcept { return my_task{handle_t::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        final_awaiter final_suspend() noexcept { return {}; }

        template <class U>
        auto await_transform(my_task<U>&& task) noexcept
        {
            struct awaiter {
                my_task<U> child;

                bool await_ready() const noexcept { return false; }
                std::coroutine_handle<> await_suspend(std::coroutine_handle<> parent) noexcept
                {
                    child.handle.promise().continuation = parent;
                    return child.handle;
                }
                U await_resume()
                {
                    if (child.handle.promise().error) std::rethrow_exception(child.handle.promise().error);
                    return child.handle.promise().value;
                }
            };
            return awaiter{std::move(task)};
        }

        template <class Sender>
            requires ex::sender<std::remove_cvref_t<Sender>>
        sender_awaitable<std::remove_cvref_t<Sender>> await_transform(Sender&& sender)
        {
            return sender_awaitable<std::remove_cvref_t<Sender>>{std::forward<Sender>(sender)};
        }

        void return_value(T v) noexcept { value = v; }
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    explicit my_task(handle_t h) noexcept : handle(h) {}
    my_task(my_task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
    my_task(const my_task&) = delete;
    ~my_task()
    {
        if (handle) handle.destroy();
    }

    template <class Receiver>
    struct op_state {
        my_task task;
        Receiver receiver;

        void start() noexcept
        {
            task.handle.resume();
            auto& promise = task.handle.promise();
            if (promise.error) {
                ex::set_error(std::move(receiver), promise.error);
            } else {
                ex::set_value(std::move(receiver), promise.value);
            }
        }
    };

    template <class Receiver>
    op_state<Receiver> connect(Receiver receiver) && { return {std::move(*this), std::move(receiver)}; }

    handle_t handle;
};
