#pragma once
#include <coroutine>
#include <stdexcept>
#include <utility>

// Single-threaded controlled fixture. No asynchronous callback survives the test.
struct manual_event {
    std::coroutine_handle<> waiter{};
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept { waiter = h; }
    void await_resume() const noexcept {}
    void set() {
        if (!waiter) throw std::runtime_error("child did not register its event");
        std::exchange(waiter, {}).resume();
    }
};
