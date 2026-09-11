#pragma once

#include <stdexec/execution.hpp>

#include <coroutine>
#include <exception>

namespace ex = stdexec;

struct stopped_error : std::exception {
    const char* what() const noexcept override { return "bad constant"; }
};

template <class Sender>
class sender_awaitable {
public:
    explicit sender_awaitable(Sender) {}
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    int await_resume() noexcept { return 21; }
};

