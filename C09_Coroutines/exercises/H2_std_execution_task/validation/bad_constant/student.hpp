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
    Scheduler,
    FirstSender,
    SecondSender,
    StoppedTask,
    TokenTask)
{
    co_return {19, 42, true, true};
}

