// H-2 starter: map the checks a real stdexec::task reference must perform.

#include <stdexec/execution.hpp>

#include <cstdio>
#include <thread>

namespace ex = stdexec;

struct task_observations {
    int sender_value = 0;
    bool scheduler_changed_thread = false;
    int when_all_sum = 0;
    bool stopped_became_empty_optional = false;
    bool stop_token_visible = false;
};

template <class Scheduler>
task_observations run_task_observations(Scheduler scheduler)
{
    (void)scheduler;
    // TODO:
    // - write a stdexec::task<int> that co_awaits two senders.
    // - use exec::static_thread_pool + starts_on and compare thread ids.
    // - co_await when_all(...) and verify both child values are present.
    // - turn a stopped child into std::optional with stopped_as_optional.
    // - co_await get_stop_token() to observe task environment propagation.
    return {};
}

int main()
{
    auto r = ex::sync_wait(ex::just(1));
    if (r) {
        auto [value] = *r;
        std::printf("starter smoke: just -> %d\n", value);
    }
    std::puts("TODO: implement run_task_observations; compare with solution.cpp.");
    return 0;
}
