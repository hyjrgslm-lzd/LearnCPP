#include "coroutine_study/lazy_task.hpp"
#include "test_check.hpp"

#include <atomic>

using coroutine_study::lazy_task;
using coroutine_study::sync_wait;

lazy_task<int> immediate(std::atomic<int>& calls) {
    ++calls;
    co_return 9;
}

struct resume_now {
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept { h.resume(); }
    void await_resume() noexcept {}
};

lazy_task<int> reentrant_suspend(std::atomic<int>& trace) {
    ++trace;
    co_await resume_now{};
    ++trace;
    co_return 11;
}

int main() {
    std::atomic<int> calls = 0;
    auto task = immediate(calls);
    check(calls == 0);
    check(sync_wait(std::move(task)) == 9);
    check(calls == 1);
    check(!task.valid());

    std::atomic<int> trace = 0;
    check(sync_wait(reentrant_suspend(trace)) == 11);
    check(trace == 2);
}
