#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <folly/coro/safe/AsyncClosure.h>
#include <folly/coro/safe/NowTask.h>
#include <folly/coro/safe/SafeTask.h>

#include <iostream>

struct i3_trace {
    int value_tasks = 0;
    int now_tasks = 0;
    int cleanup_tasks = 0;
};

folly::coro::value_task<int> value_task_todo(int value, i3_trace* trace)
{
    // TODO: keep arguments value-semantic; do not take raw references.
    // Increment trace only when this coroutine body actually runs.
    (void)value;
    (void)trace;
    co_return 0;
}

folly::coro::now_task<int> now_task_todo(i3_trace* trace)
{
    // TODO: await this in the full expression that creates it.
    (void)trace;
    co_return 0;
}

folly::coro::co_cleanup_safe_task<void> cleanup_safe_todo(i3_trace* trace)
{
    // TODO: use this shape for work safe to schedule during scope/closure cleanup.
    (void)trace;
    co_return;
}

folly::coro::Task<void> task_todo(i3_trace* trace)
{
    co_await value_task_todo(5, trace);
    co_await now_task_todo(trace);
    co_await cleanup_safe_todo(trace);
}

int main()
{
    i3_trace trace;
    int left = folly::coro::blocking_wait(value_task_todo(17, &trace));
    int right = folly::coro::blocking_wait(value_task_todo(25, &trace));
    int now = folly::coro::blocking_wait(now_task_todo(&trace));
    folly::coro::blocking_wait(task_todo(&trace));
    if (left + right != 42 || now != 7 || trace.value_tasks != 3 || trace.now_tasks != 2 || trace.cleanup_tasks != 1) {
        std::cout << "student check failed: values/trace are "
                  << left << "/" << right << "/" << now << " trace="
                  << trace.value_tasks << "/" << trace.now_tasks << "/" << trace.cleanup_tasks
                  << ", expected 17/25/7 and 3/2/1\n";
        return 1;
    }
    std::cout << "I3 student check passed.\n";
}
