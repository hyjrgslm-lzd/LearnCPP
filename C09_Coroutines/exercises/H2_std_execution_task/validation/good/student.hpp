#pragma once

#include <stdexec/execution.hpp>

#include <optional>

namespace ex = stdexec;

struct task_observations {
    int sender_value = 0;
    int when_all_sum = 0;
    bool stopped_became_empty_optional = false;
    bool stop_token_visible = false;
};

template <class Scheduler, class FirstSender, class SecondSender, class StoppedTask, class TokenTask>
stdexec::task<task_observations> run_task_observations(
    Scheduler scheduler,
    FirstSender first,
    SecondSender second,
    StoppedTask stopped,
    TokenTask token_task)
{
    task_observations out;
    out.sender_value = co_await std::move(first);
    auto [left, right] = co_await ex::when_all(
        ex::just(out.sender_value),
        ex::starts_on(scheduler, std::move(second)));
    out.when_all_sum = left + right;

    auto stopped_value = co_await ex::stopped_as_optional(std::move(stopped));
    out.stopped_became_empty_optional = !stopped_value.has_value();

    out.stop_token_visible = co_await std::move(token_task);
    co_return out;
}
