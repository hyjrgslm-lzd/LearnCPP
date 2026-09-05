#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

#include <cstdio>
#include <optional>
#include <thread>
#include <tuple>

namespace ex = stdexec;

template <ex::sender S1, ex::sender S2>
stdexec::task<int> await_two_senders(S1 first, S2 second)
{
    co_await static_cast<S2&&>(second);
    co_return co_await static_cast<S1&&>(first);
}

template <ex::scheduler Scheduler>
stdexec::task<std::thread::id> hop_to_scheduler(Scheduler scheduler)
{
    co_return co_await ex::starts_on(
        scheduler,
        ex::just() | ex::then([] { return std::this_thread::get_id(); }));
}

template <ex::scheduler Scheduler>
stdexec::task<int> fan_in_on_scheduler(Scheduler scheduler)
{
    auto [left, right] = co_await ex::when_all(
        ex::starts_on(scheduler, ex::just(20) | ex::then([](int value) { return value + 1; })),
        ex::starts_on(scheduler, ex::just(30) | ex::then([](int value) { return value + 2; })));
    co_return left + right;
}

stdexec::task<std::optional<int>> stopped_child_to_optional()
{
    co_return co_await ex::stopped_as_optional(
        await_two_senders(ex::just(7), ex::just_stopped()));
}

stdexec::task<bool> task_observes_stop_environment()
{
    auto token = co_await ex::get_stop_token();
    co_return token.stop_possible();
}

int main()
{
    exec::static_thread_pool pool{1};
    const auto main_thread = std::this_thread::get_id();
    const auto scheduler = pool.get_scheduler();

    auto value = ex::sync_wait(await_two_senders(ex::just(42), ex::just()));
    if (!value || std::get<0>(*value) != 42) {
        return 1;
    }

    auto worker_thread = ex::sync_wait(hop_to_scheduler(scheduler));
    if (!worker_thread || std::get<0>(*worker_thread) == main_thread) {
        return 2;
    }

    auto fan_in = ex::sync_wait(fan_in_on_scheduler(scheduler));
    if (!fan_in || std::get<0>(*fan_in) != 53) {
        return 3;
    }

    auto stopped = ex::sync_wait(stopped_child_to_optional());
    if (!stopped || std::get<0>(*stopped).has_value()) {
        return 4;
    }

    auto has_stop_env = ex::sync_wait(task_observes_stop_environment());
    if (!has_stop_env || !std::get<0>(*has_stop_env)) {
        return 5;
    }

    std::printf("task_value=%d worker_changed=%d when_all_sum=%d stopped=nullopt stop_env=%d\n",
                std::get<0>(*value),
                std::get<0>(*worker_thread) != main_thread,
                std::get<0>(*fan_in),
                std::get<0>(*has_stop_env));
    return 0;
}
