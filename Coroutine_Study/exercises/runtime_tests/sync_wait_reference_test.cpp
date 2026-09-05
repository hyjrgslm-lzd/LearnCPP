#include "coroutine_study/runtime.hpp"
#include "test_check.hpp"

#include <atomic>
#include <stdexcept>
#include <thread>

using namespace coroutine_study;

lazy_task<int> scheduled(run_loop& loop, std::atomic<int>& starts) {
    ++starts;
    co_await loop.schedule();
    co_return 5;
}

lazy_task<int> immediate_value(std::atomic<int>& starts) {
    ++starts;
    co_return 8;
}

lazy_task<int> suspended_once(std::atomic<int>& starts) {
    ++starts;
    co_await std::suspend_always{};
    co_return 6;
}

int main() {
    run_loop loop;
    std::jthread worker([&] { loop.run(); });
    std::atomic<int> starts = 0;

    auto task = scheduled(loop, starts);
    check(starts == 0);
    check(sync_wait(std::move(task)) == 5);
    check(starts == 1);

    auto started = suspended_once(starts);
    started.start();
    check(starts == 2);
    bool sync_wait_started_suspended_threw = false;
    try {
        (void)sync_wait(std::move(started));
    } catch (const std::logic_error&) {
        sync_wait_started_suspended_threw = true;
    }
    check(sync_wait_started_suspended_threw);

    auto done_task = immediate_value(starts);
    done_task.start();
    check(done_task.done());
    bool sync_wait_done_started_threw = false;
    try {
        (void)sync_wait(std::move(done_task));
    } catch (const std::logic_error&) {
        sync_wait_done_started_threw = true;
    }
    check(sync_wait_done_started_threw);

    auto twice = immediate_value(starts);
    twice.start();
    bool second_start_threw = false;
    try {
        twice.start();
    } catch (const std::logic_error&) {
        second_start_threw = true;
    }
    check(second_start_threw);

    loop.finish();
}
