#pragma once

#include <stdexec/execution.hpp>

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
    (void)scheduler;
    (void)first;
    (void)second;
    (void)stopped;
    (void)token_task;
    // TODO:
    // - co_await first to get sender_value.
    // - co_await when_all(just(sender_value), starts_on(scheduler, second)).
    // - co_await stopped_as_optional(stopped); this completes as nullopt and does not continue through the stopped child.
    // - co_await token_task; the checker task body performs the actual get_stop_token query.
    co_return {};
}
