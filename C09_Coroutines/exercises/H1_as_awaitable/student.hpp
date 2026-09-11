#pragma once

#include <stdexec/execution.hpp>

#include <coroutine>
#include <exception>
#include <optional>

namespace ex = stdexec;

struct stopped_error : std::exception {
    const char* what() const noexcept override { return "sender completed with set_stopped"; }
};

template <class Sender>
class sender_awaitable {
public:
    explicit sender_awaitable(Sender sender) : sender_(static_cast<Sender&&>(sender)) {}

    bool await_ready() const noexcept { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting)
    {
        // TODO:
        // 1. connect sender_ to a receiver that stores value/error/stopped.
        // 2. keep the operation_state inside this awaitable.
        // 3. start it and handle the "completed before await_suspend returns" case.
        // 4. return the continuation only when asynchronous completion really happens.
        (void)awaiting;
        return std::noop_coroutine();
    }

    int await_resume()
    {
        // TODO: return value, rethrow error, or throw stopped_error.
        return 0;
    }

private:
    Sender sender_;
    std::optional<int> value_;
    std::exception_ptr error_;
    bool stopped_ = false;
};
