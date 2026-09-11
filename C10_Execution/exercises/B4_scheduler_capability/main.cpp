#include <c10/test.hpp>
#include <solution.hpp>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <thread>

namespace {
void check_probe(int threads, int tasks) {
  auto result = c10_b4::run_scheduler_probe(threads, tasks);
  c10::require(result.requested_threads == threads, "thread pool request recorded");
  c10::require(result.completed_tasks == tasks, "all requested scheduled tasks complete");
  c10::require(result.caller_thread_id == std::this_thread::get_id(), "caller thread is recorded");
  if (tasks > 0) {
    c10::require(!result.worker_thread_ids.empty(), "scheduled work observes worker context");
    c10::require(std::none_of(result.worker_thread_ids.begin(), result.worker_thread_ids.end(),
                              [&](auto id) { return id == result.caller_thread_id; }),
                 "scheduled work does not run inline on caller thread");
  } else {
    c10::require(result.worker_thread_ids.empty(), "zero tasks do not invent worker observations");
  }
  c10::require(result.scheduler_copies_remain_usable, "scheduler copies can create later work");
}
} // namespace

int main() {
  return c10::test_main([] {
    for (int repeat = 0; repeat < 2; ++repeat) {
      for (int tasks : std::array{0, 1, 3, 8, 17}) {
        check_probe(1, tasks);
      }
      check_probe(4, 17);
    }
    bool rejected = false;
    try {
      (void)c10_b4::run_scheduler_probe(0, 1);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    c10::require(rejected, "thread_count lower bound is validated");
    rejected = false;
    try {
      (void)c10_b4::run_scheduler_probe(65, 1);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    c10::require(rejected, "thread_count upper bound is validated");
    rejected = false;
    try {
      (void)c10_b4::run_scheduler_probe(1, -1);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    c10::require(rejected, "task_count lower bound is validated");
    rejected = false;
    try {
      (void)c10_b4::run_scheduler_probe(1, 1025);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    c10::require(rejected, "task_count upper bound is validated");
  });
}
