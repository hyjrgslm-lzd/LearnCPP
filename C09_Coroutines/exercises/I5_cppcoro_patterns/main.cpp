#include <cppcoro/cancellation_source.hpp>
#include <cppcoro/cancellation_token.hpp>
#include <cppcoro/generator.hpp>
#include <cppcoro/shared_task.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/task.hpp>
#include <cppcoro/when_all.hpp>

#include <iostream>

cppcoro::generator<int> generator_todo()
{
    // TODO: co_yield a finite sequence and inspect iterator-driven resume.
    if (false) {
        co_yield 0;
    }
}

cppcoro::task<int> task_todo()
{
    // TODO: compute a value lazily, then drive it from sync_wait.
    co_return 0;
}

cppcoro::shared_task<int> shared_task_todo()
{
    // TODO: await the same shared_task twice in the reference shape.
    co_return 0;
}

cppcoro::task<void> cancellation_todo(cppcoro::cancellation_token token)
{
    // TODO: observe token.is_cancellation_requested() and register a callback.
    (void)token;
    co_return;
}

cppcoro::task<void> scheduler_todo(cppcoro::static_thread_pool& pool)
{
    // TODO: co_await pool.schedule() and prove resume happens on a worker.
    (void)pool;
    co_return;
}

int main()
{
    cppcoro::cancellation_source source;
    cppcoro::static_thread_pool pool{1};

    int generated_sum = 0;
    for (int value : generator_todo()) generated_sum += value;

    source.request_cancellation();
    int task_value = cppcoro::sync_wait(task_todo());
    int shared_value = cppcoro::sync_wait(shared_task_todo());
    cppcoro::sync_wait(cancellation_todo(source.token()));
    cppcoro::sync_wait(scheduler_todo(pool));

    if (generated_sum != 6 || task_value != 42 || shared_value != 42) {
        std::cout << "student check failed: generator/task/shared_task observed "
                  << generated_sum << "/" << task_value << "/" << shared_value
                  << ", expected 6/42/42\n";
        return 1;
    }
    std::cout << "I5 student check passed.\n";
}
