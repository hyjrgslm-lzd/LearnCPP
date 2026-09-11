#include "student.hpp"

#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

#include <cstdio>
#include <thread>

namespace ex = stdexec;

stdexec::task<int> stopped_child(int* stopped_started)
{
    ++*stopped_started;
    co_await ex::just_stopped();
    co_return 7;
}

stdexec::task<bool> token_child(int* token_queries)
{
    ++*token_queries;
    auto token = co_await ex::get_stop_token();
    co_return token.stop_possible();
}

int main()
{
    exec::static_thread_pool pool{1};
    auto main_thread = std::this_thread::get_id();
    int first_started = 0;
    int worker_ran = 0;
    int stopped_started = 0;
    int token_queries = 0;

    auto first = ex::just(17) | ex::then([&](int v) noexcept {
        ++first_started;
        return v + 2;
    });
    auto second = ex::just(20) | ex::then([&](int v) noexcept {
        if (std::this_thread::get_id() != main_thread) ++worker_ran;
        return v + 3;
    });

    auto result = ex::sync_wait(run_task_observations(
        pool.get_scheduler(),
        std::move(first),
        std::move(second),
        stopped_child(&stopped_started),
        token_child(&token_queries)));
    if (!result) {
        std::puts("student check failed: task produced no value");
        return 1;
    }

    const auto observed = std::get<0>(*result);
    const bool ok = observed.sender_value == 19
        && observed.when_all_sum == 42
        && observed.stopped_became_empty_optional
        && observed.stop_token_visible
        && first_started == 1
        && worker_ran == 1
        && stopped_started == 1
        && token_queries == 1;
    if (!ok) {
        std::printf(
            "student check failed: value=%d sum=%d stopped=%d stop=%d counters=%d/%d/%d/%d, expected 19/42/1/1 and 1/1/1/1\n",
            observed.sender_value,
            observed.when_all_sum,
            observed.stopped_became_empty_optional,
            observed.stop_token_visible,
            first_started,
            worker_ran,
            stopped_started,
            token_queries);
        return 1;
    }
    std::puts("H2 student check passed.");
}
