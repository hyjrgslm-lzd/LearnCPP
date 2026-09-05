#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <folly/coro/safe/AsyncClosure.h>
#include <folly/coro/safe/NowTask.h>
#include <folly/coro/safe/SafeTask.h>

#include <iostream>

folly::coro::value_task<int> value_task_todo(int value)
{
    // TODO: keep arguments value-semantic; do not take raw references.
    co_return value;
}

folly::coro::now_task<int> now_task_todo()
{
    // TODO: await this in the full expression that creates it.
    co_return 0;
}

folly::coro::co_cleanup_safe_task<void> cleanup_safe_todo()
{
    // TODO: use this shape for work safe to schedule during scope/closure cleanup.
    co_return;
}

folly::coro::Task<void> task_todo()
{
    co_await value_task_todo(0);
    co_await now_task_todo();
    co_await cleanup_safe_todo();
}

int main()
{
    std::cout << "I3 Folly starter skeleton compiled. Wire executor/async_closure TODOs next.\n";
}
