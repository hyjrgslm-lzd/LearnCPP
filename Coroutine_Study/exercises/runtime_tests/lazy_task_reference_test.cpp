#include "coroutine_study/lazy_task.hpp"
#include "test_check.hpp"

#include <memory>
#include <stdexcept>

using coroutine_study::lazy_task;
using coroutine_study::sync_wait;

lazy_task<int> value_task() {
    co_return 42;
}

lazy_task<std::unique_ptr<int>> move_only_task() {
    co_return std::make_unique<int>(7);
}

lazy_task<int> child() {
    co_return 20;
}

lazy_task<int> parent() {
    int v = co_await child();
    co_return v + 1;
}

lazy_task<int> failing() {
    throw std::runtime_error("boom");
    co_return 0;
}

lazy_task<int> await_task(lazy_task<int> task) {
    int v = co_await std::move(task);
    co_return v;
}

lazy_task<int> suspended_child() {
    co_await std::suspend_always{};
    co_return 30;
}

int main() {
    auto task = value_task();
    check(task.valid());
    check(sync_wait(std::move(task)) == 42);
    check(!task.valid());

    auto ptr = sync_wait(move_only_task());
    check(*ptr == 7);
    check(sync_wait(parent()) == 21);

    coroutine_study::lazy_task_fail_next_allocation = true;
    auto empty = value_task();
    bool await_empty_threw = false;
    try {
        (void)sync_wait(await_task(std::move(empty)));
    } catch (const std::bad_alloc&) {
        await_empty_threw = true;
    }
    check(await_empty_threw);

    auto started_child = suspended_child();
    started_child.start();
    bool await_started_threw = false;
    try {
        (void)sync_wait(await_task(std::move(started_child)));
    } catch (const std::logic_error&) {
        await_started_threw = true;
    }
    check(await_started_threw);

    auto completed_child = child();
    completed_child.start();
    check(completed_child.done());
    bool await_completed_started_threw = false;
    try {
        (void)sync_wait(await_task(std::move(completed_child)));
    } catch (const std::logic_error&) {
        await_completed_started_threw = true;
    }
    check(await_completed_started_threw);

    bool threw = false;
    try {
        (void)sync_wait(failing());
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw);
}
