#include <c10/test.hpp>
#include <solution.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include <string>
#include <thread>
#include <tuple>

namespace ex = stdexec;

namespace {
void check_environment_query_is_not_payload(int task_id, bool stop_requested,
                                            const std::string &payload) {
  exec::static_thread_pool pool(1);
  ex::inplace_stop_source stop;
  if (stop_requested) {
    stop.request_stop();
  }
  auto env = ex::env{ex::prop{c10_c2_9::get_task_id, task_id},
                     ex::prop{ex::get_stop_token, stop.get_token()},
                     ex::prop{ex::get_scheduler, pool.get_scheduler()}};
  auto result =
      ex::sync_wait(ex::write_env(c10_c2_9::read_runtime_context(std::string{payload}), env));
  c10::require(result.has_value(), "environment sender completes");
  auto snapshot = std::get<0>(*result);
  c10::require(snapshot.payload == payload, "payload forwarded separately");
  c10::require(snapshot.task_id == task_id, "task id queried from environment");
  c10::require(snapshot.stop_requested == stop_requested,
               "stop token state queried from environment");
  c10::require(snapshot.start_thread == std::this_thread::get_id(),
               "sender starts on caller before scheduler follow-up");
  c10::require(snapshot.followup_thread != snapshot.start_thread,
               "follow-up uses queried scheduler");
}
} // namespace

int main() {
  return c10::test_main([] {
    check_environment_query_is_not_payload(42, false, "payload-record");
    check_environment_query_is_not_payload(7, true, "other-record");
  });
}
