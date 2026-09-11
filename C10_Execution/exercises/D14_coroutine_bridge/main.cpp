#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

#include <concepts>
#include <string>
#include <thread>

namespace ex = stdexec;

namespace {
struct probe_promise : ex::with_awaitable_senders<probe_promise> {
  auto get_env() const noexcept { return ex::env<>{}; }
};
} // namespace

int main() {
  return c10::test_main([] {
    auto sender = ex::sync_wait(c10_d14::sender_graph(21));
    auto sender_fresh = ex::sync_wait(c10_d14::sender_graph(5));
    auto coroutine = ex::sync_wait(c10_d14::coroutine_task(21));
    auto task_value = ex::sync_wait(c10_d14::stdexec_task());
    auto sender_stopped = ex::sync_wait(c10_d14::sender_stopped_status());
    auto coroutine_stopped = ex::sync_wait(c10_d14::coroutine_stopped_status());

    exec::static_thread_pool pool{1};
    auto scheduler = pool.get_scheduler();
    auto caller = std::this_thread::get_id();
    auto sender_thread = ex::sync_wait(c10_d14::sender_switch_thread(scheduler));
    auto coroutine_thread = ex::sync_wait(c10_d14::coroutine_switch_thread(scheduler));

    c10::require(sender && std::get<0>(*sender) == "42", "sender graph transforms record");
    c10::require(sender_fresh && std::get<0>(*sender_fresh) == "10",
                 "sender graph consumes fresh input");
    c10::require(coroutine && std::get<0>(*coroutine) == std::get<0>(*sender),
                 "coroutine result matches sender graph");
    c10::require(task_value && std::get<0>(*task_value) == 5, "stdexec task is consumed as sender");
    c10::require(sender_stopped && std::get<0>(*sender_stopped) == "stopped",
                 "sender graph maps stopped to explicit status");
    c10::require(coroutine_stopped && std::get<0>(*coroutine_stopped) == "stopped",
                 "coroutine maps stopped to explicit status");
    c10::require(sender_thread && std::get<0>(*sender_thread) != caller,
                 "sender graph switches to scheduler thread");
    c10::require(coroutine_thread && std::get<0>(*coroutine_thread) != caller,
                 "coroutine explicitly switches to scheduler thread");
    c10::require(std::get<0>(*sender_thread) == std::get<0>(*coroutine_thread),
                 "sender and coroutine observe the same scheduler execution source");
    static_assert(
        requires(probe_promise promise) { ex::as_awaitable(c10_d14::sender_graph(1), promise); });
  });
}
