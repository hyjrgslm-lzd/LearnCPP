#pragma once

#include "mini_ref/mini.hpp"

#include <stdexec/execution.hpp>

#include <atomic>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

namespace mini_ref {

template <typename Sender>
class stdexec_awaitable {
    enum phase : int { starting, suspended, completed, abandoned };

    struct base_state {
        std::coroutine_handle<> caller;
        std::atomic<int> phase_{starting};
        std::variant<std::monostate, int, std::exception_ptr, std::monostate> result;

        void set_value(int value) noexcept {
            result.template emplace<1>(value);
            complete();
        }

        void set_error(std::exception_ptr error) noexcept {
            result.template emplace<2>(std::move(error));
            complete();
        }

        void set_stopped() noexcept {
            result.template emplace<3>();
            complete();
        }

        void complete() noexcept {
            int old = phase_.exchange(completed, std::memory_order_acq_rel);
            if (old == suspended) caller.resume();
        }
    };

public:
    struct receiver {
        std::shared_ptr<base_state> state;

        using receiver_concept = stdexec::receiver_t;

        friend stdexec::empty_env tag_invoke(stdexec::get_env_t, const receiver&) noexcept {
            return {};
        }

        friend void tag_invoke(stdexec::set_value_t, receiver&& self, int value) noexcept {
            auto keep = std::move(self.state);
            keep->set_value(value);
        }

        friend void tag_invoke(stdexec::set_error_t, receiver&& self, std::exception_ptr error) noexcept {
            auto keep = std::move(self.state);
            keep->set_error(std::move(error));
        }

        friend void tag_invoke(stdexec::set_stopped_t, receiver&& self) noexcept {
            auto keep = std::move(self.state);
            keep->set_stopped();
        }
    };

private:
    using operation_t = decltype(stdexec::connect(std::declval<Sender>(), receiver{}));

    struct state : base_state {
        explicit state(Sender sender) : sender_(std::move(sender)) {}
        Sender sender_;
        std::optional<operation_t> operation_;
    };

public:
    explicit stdexec_awaitable(Sender sender) : state_(std::make_shared<state>(std::move(sender))) {}
    stdexec_awaitable(stdexec_awaitable&&) noexcept = default;
    stdexec_awaitable(const stdexec_awaitable&) = delete;
    stdexec_awaitable& operator=(stdexec_awaitable&&) = delete;
    stdexec_awaitable& operator=(const stdexec_awaitable&) = delete;

    ~stdexec_awaitable() {
        if (!state_) return;
        int expected = suspended;
        state_->phase_.compare_exchange_strong(
            expected, abandoned, std::memory_order_acq_rel, std::memory_order_acquire);
    }

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> caller) {
        auto keep = state_;
        keep->operation_.emplace(stdexec::connect(std::move(keep->sender_), receiver{keep}));
        stdexec::start(*keep->operation_);
        keep->caller = caller;

        int expected = starting;
        if (keep->phase_.compare_exchange_strong(
                expected, suspended, std::memory_order_acq_rel, std::memory_order_acquire)) {
            return true;
        }
        return false;
    }

    int await_resume() {
        auto keep = state_;
        if (keep->result.index() == 2) std::rethrow_exception(std::get<2>(keep->result));
        if (keep->result.index() == 3) throw std::runtime_error{"sender stopped"};
        return std::get<1>(keep->result);
    }

private:
    std::shared_ptr<state> state_;
};

template <typename Sender>
auto as_stdexec_awaitable(Sender sender) {
    return stdexec_awaitable<Sender>{std::move(sender)};
}

} // namespace mini_ref
