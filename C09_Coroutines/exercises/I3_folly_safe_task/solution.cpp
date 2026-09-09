#include <folly/coro/BlockingWait.h>
#include <folly/coro/CurrentExecutor.h>
#include <folly/coro/Task.h>
#include <folly/coro/safe/AsyncClosure.h>
#include <folly/coro/safe/NowTask.h>
#include <folly/coro/safe/SafeTask.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <coroutine_study/exercise_check.hpp>

#include <iostream>
#include <thread>

folly::coro::value_task<int> add_values(int a, int b)
{
    co_return a + b;
}

folly::coro::Task<int> run_safe_task_demo()
{
    int value = co_await add_values(20, 22);
    co_return value;
}

folly::coro::now_task<int> immediate_value()
{
    co_return 7;
}

folly::coro::Task<std::thread::id> executor_bound_task()
{
    co_await folly::coro::co_current_executor;
    co_return std::this_thread::get_id();
}

folly::coro::Task<int> async_closure_demo()
{
    auto closure = folly::coro::async_closure(
        folly::bind::args{5},
        [](int x) -> folly::coro::closure_task<int> { co_return x + 1; });
    co_return co_await std::move(closure);
}

folly::coro::co_cleanup_safe_task<int> cleanup_safe_value(int value)
{
    co_return value;
}

folly::coro::Task<int> cleanup_safe_demo()
{
    co_return co_await cleanup_safe_value(9);
}

int main()
{
    folly::CPUThreadPoolExecutor executor{1};

    const int result = folly::coro::blocking_wait(run_safe_task_demo());
    coroutine_study::check(result == 42, "value_task result mismatch");

    const int now = folly::coro::blocking_wait(immediate_value());
    coroutine_study::check(now == 7, "now_task result mismatch");

    auto worker = folly::coro::blocking_wait(
        folly::coro::co_withExecutor(&executor, executor_bound_task()));
    coroutine_study::check(worker != std::this_thread::get_id(), "Task did not run on executor");

    const int closure = folly::coro::blocking_wait(async_closure_demo());
    coroutine_study::check(closure == 6, "async_closure result mismatch");

    const int cleanup = folly::coro::blocking_wait(cleanup_safe_demo());
    coroutine_study::check(cleanup == 9, "co_cleanup_safe_task result mismatch");

    std::cout << "I3 reference passed: value_task, now_task, executor-bound Task, async_closure, co_cleanup_safe_task\n";
}
