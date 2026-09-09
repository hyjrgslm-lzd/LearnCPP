#include <coroutine_study/exercise_check.hpp>
#include <cppcoro/cancellation_registration.hpp>
#include <cppcoro/cancellation_source.hpp>
#include <cppcoro/cancellation_token.hpp>
#include <cppcoro/generator.hpp>
#include <cppcoro/schedule_on.hpp>
#include <cppcoro/shared_task.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/task.hpp>
#include <cppcoro/when_all.hpp>

#include <iostream>
#include <atomic>
#include <thread>
#include <tuple>
#include <vector>

cppcoro::generator<int> range(int first, int last)
{
    for (int i = first; i != last; ++i) {
        co_yield i;
    }
}

cppcoro::task<int> sum_range(int first, int last)
{
    int sum = 0;
    for (int value : range(first, last)) {
        sum += value;
    }
    co_return sum;
}

cppcoro::shared_task<int> cached_answer()
{
    co_return 42;
}

cppcoro::task<bool> scheduler_handoff(cppcoro::static_thread_pool& pool)
{
    const auto before = std::this_thread::get_id();
    co_await pool.schedule();
    co_return before != std::this_thread::get_id();
}

cppcoro::task<bool> cancellation_demo(cppcoro::cancellation_token token)
{
    bool callback_seen = false;
    cppcoro::cancellation_registration registration{token, [&] { callback_seen = true; }};
    co_return token.is_cancellation_requested() && callback_seen;
}

cppcoro::task<int> demo(cppcoro::static_thread_pool& pool)
{
    auto [a, b] = co_await cppcoro::when_all(sum_range(1, 4), sum_range(4, 7));
    coroutine_study::check(a == 6, "first sum mismatch");
    coroutine_study::check(b == 15, "second sum mismatch");

    auto shared = cached_answer();
    coroutine_study::check(co_await shared == 42, "first shared_task await mismatch");
    coroutine_study::check(co_await shared == 42, "second shared_task await mismatch");

    bool moved_threads = co_await scheduler_handoff(pool);
    coroutine_study::check(moved_threads, "schedule() did not resume on pool worker");

    cppcoro::cancellation_source source;
    source.request_cancellation();
    coroutine_study::check(co_await cancellation_demo(source.token()), "cancellation callback/token mismatch");

    co_return a + b;
}

int main()
{
    cppcoro::static_thread_pool pool{1};
    const int total = cppcoro::sync_wait(demo(pool));
    coroutine_study::check(total == 21, "when_all total mismatch");
    std::cout << "I5 reference passed: cppcoro task/shared_task/generator/when_all/scheduler\n";
}
