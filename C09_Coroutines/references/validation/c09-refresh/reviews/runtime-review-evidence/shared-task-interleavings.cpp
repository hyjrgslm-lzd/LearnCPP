#include "mini_ref/mini.hpp"

#include <atomic>
#include <coroutine>
#include <cstdlib>
#include <iostream>
#include <latch>
#include <stdexcept>
#include <thread>

static std::coroutine_handle<> source_handle{};

struct hold_source {
    std::latch* ready{};
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept {
        source_handle = h;
        ready->count_down();
    }
    void await_resume() const noexcept {}
};

static mini_ref::task<int> sync_value() {
    co_return 11;
}

static mini_ref::task<int> sync_error() {
    throw std::runtime_error{"sync source failed"};
    co_return 0;
}

static mini_ref::task<int> cross_thread_value(std::latch& ready) {
    co_await hold_source{&ready};
    co_return 17;
}

static mini_ref::task<int> await_value(mini_ref::shared_task<int> shared, std::atomic<int>& resumes) {
    int value = co_await shared;
    ++resumes;
    co_return value;
}

static mini_ref::task<int> await_error(mini_ref::shared_task<int> shared, std::atomic<int>& catches) {
    try {
        (void)co_await shared;
    } catch (const std::runtime_error&) {
        ++catches;
        co_return 23;
    }
    co_return -1;
}

int main() {
    std::atomic<int> sync_resumes{0};
    auto sync_result = mini_ref::sync_wait(await_value(mini_ref::share(sync_value()), sync_resumes));
    if (!sync_result || std::get<0>(*sync_result) != 11 || sync_resumes.load() != 1) std::abort();

    std::atomic<int> catches{0};
    auto error_result = mini_ref::sync_wait(await_error(mini_ref::share(sync_error()), catches));
    if (!error_result || std::get<0>(*error_result) != 23 || catches.load() != 1) std::abort();

    std::atomic<int> async_resumes{0};
    std::latch ready{1};
    source_handle = {};
    std::jthread resumer{[&] {
        ready.wait();
        auto h = std::exchange(source_handle, {});
        if (!h) std::abort();
        h.resume();
    }};
    auto async_result = mini_ref::sync_wait(await_value(mini_ref::share(cross_thread_value(ready)), async_resumes));
    resumer.join();
    if (!async_result || std::get<0>(*async_result) != 17 || async_resumes.load() != 1) std::abort();

    std::cout << "shared_task interleavings: sync value, sync error, cross-thread value checked\n";
}
