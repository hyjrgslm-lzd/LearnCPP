#include "mini_ref/mini.hpp"
#include "async_test_helpers.hpp"

#include <atomic>
#include <cstdlib>
#include <latch>
#include <memory>
#include <optional>
#include <stdexcept>

static mini_ref::task<int> value() {
    co_return 7;
}

static mini_ref::task<void> done_void(bool& reached) {
    reached = true;
    co_return;
}

static mini_ref::task<int> fail() {
    throw std::runtime_error{"boom"};
    co_return 0;
}

static mini_ref::task<void> fail_void() {
    throw std::runtime_error{"void boom"};
    co_return;
}

static mini_ref::task<int> cross_thread_before_suspend_returns(mini_ref_test::async_threads& threads) {
    co_await mini_ref_test::resume_before_suspend_returns{threads};
    co_return 41;
}

static mini_ref::task<int> delayed_value(mini_ref_test::async_threads& threads,
                                         std::latch& started,
                                         std::latch& release) {
    co_await mini_ref_test::resume_after_release{threads, started, release};
    co_return 42;
}

static mini_ref::task<void> delayed_void(mini_ref_test::async_threads& threads,
                                         std::latch& started,
                                         std::latch& release,
                                         bool& reached) {
    co_await mini_ref_test::resume_after_release{threads, started, release};
    reached = true;
    co_return;
}

static mini_ref::task<int> counted_start(std::atomic<int>& starts) {
    ++starts;
    co_return 5;
}

static mini_ref::task<int> counted_start_suspended(mini_ref::run_loop& loop, std::atomic<int>& starts) {
    ++starts;
    co_await loop.schedule();
    co_return 5;
}

struct frame_probe {
    bool& destroyed;
    explicit frame_probe(bool& flag) : destroyed(flag) {}
    frame_probe(const frame_probe&) = delete;
    ~frame_probe() { destroyed = true; }
};

static mini_ref::task<void> mark_frame_destroyed(std::unique_ptr<frame_probe> probe) {
    (void)probe;
    co_return;
}

int main() {
    auto ok = mini_ref::sync_wait(value());
    if (!ok || std::get<0>(*ok) != 7) std::abort();

    bool reached = false;
    auto void_ok = mini_ref::sync_wait(done_void(reached));
    if (!void_ok || !reached) std::abort();

    bool caught = false;
    try {
        (void)mini_ref::sync_wait(fail());
    } catch (const std::runtime_error&) {
        caught = true;
    }
    if (!caught) std::abort();

    caught = false;
    try {
        (void)mini_ref::sync_wait(fail_void());
    } catch (const std::runtime_error&) {
        caught = true;
    }
    if (!caught) std::abort();

    mini_ref_test::async_threads threads;
    auto inline_resume = mini_ref::sync_wait(cross_thread_before_suspend_returns(threads));
    if (!inline_resume || std::get<0>(*inline_resume) != 41) std::abort();

    std::latch value_started{1};
    std::latch value_release{1};
    std::latch value_done{1};
    std::optional<std::tuple<int>> delayed_int;
    threads.spawn([&] {
        delayed_int = mini_ref::sync_wait(delayed_value(threads, value_started, value_release)).value();
        value_done.count_down();
    });
    value_started.wait();
    value_release.count_down();
    value_done.wait();
    if (std::get<0>(*delayed_int) != 42) std::abort();

    std::latch void_started{1};
    std::latch void_release{1};
    std::latch void_done{1};
    bool delayed_void_reached = false;
    threads.spawn([&] {
        (void)mini_ref::sync_wait(delayed_void(threads, void_started, void_release, delayed_void_reached));
        void_done.count_down();
    });
    void_started.wait();
    void_release.count_down();
    void_done.wait();
    if (!delayed_void_reached) std::abort();

    std::atomic<int> starts{0};
    auto once_result = mini_ref::sync_wait(counted_start(starts));
    if (!once_result || std::get<0>(*once_result) != 5 || starts != 1) std::abort();

    mini_ref::run_loop loop;
    auto once = counted_start_suspended(loop, starts);
    once.start();
    if (starts != 2) std::abort();

    bool second_start_threw = false;
    try {
        once.start();
    } catch (const std::logic_error&) {
        second_start_threw = true;
    }
    if (!second_start_threw || starts != 2) std::abort();
    loop.run();
    if (once.await_resume() != 5) std::abort();

    bool destroyed = false;
    (void)mini_ref::sync_wait(mark_frame_destroyed(std::make_unique<frame_probe>(destroyed)));
    if (!destroyed) std::abort();

    threads.join_all();
}
