#pragma once
#include "mini/task.hpp"
#include <stdexcept>
#include <stdexec/execution.hpp>
namespace mini {
template <typename Sender>
struct sender_awaitable {
    Sender sender_;
    bool await_ready() noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    int await_resume() {
        if constexpr (requires { sender_.error; }) std::rethrow_exception(sender_.error);
        else if constexpr (requires { sender_.stopped; }) return 0;
        else return sender_.value;
    }
};
template <typename Sender>
auto as_awaitable(Sender&& s) { return sender_awaitable<std::decay_t<Sender>>{std::forward<Sender>(s)}; }
} // namespace mini
