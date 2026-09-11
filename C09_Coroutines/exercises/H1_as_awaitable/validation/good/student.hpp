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

struct stopped_error : std::exception {
    const char* what() const noexcept override { return "sender stopped"; }
};

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
        if (std::holds_alternative<stopped_tag>(state.result)) throw stopped_error{};
        throw std::logic_error("sender resumed without completion");
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
