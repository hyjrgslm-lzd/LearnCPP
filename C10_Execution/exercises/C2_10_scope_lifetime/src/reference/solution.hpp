#pragma once
#include <exec/async_scope.hpp>
#include <exec/just_from.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include <atomic>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace c10_c2_10 {

namespace ex = stdexec;

struct scope_report {
  int spawned{};
  int completed{};
  int future_sum{};
  bool destroyed_after_empty{};
  std::string error_message;
  std::vector<std::string> events;
};

struct failing_result {
  failing_result() = default;
  failing_result(const failing_result &) { throw std::runtime_error("child failed"); }
};

inline auto run_scope_story() -> scope_report {
  scope_report report;
  std::mutex events_mutex;
  std::atomic<int> completed{0};
  exec::static_thread_pool pool{2};
  exec::async_scope scope;
  auto scheduler = pool.get_scheduler();

  auto remember = [&](std::string text) {
    std::lock_guard lock(events_mutex);
    report.events.push_back(std::move(text));
  };

  try {
    for (int id = 1; id <= 3; ++id) {
      scope.spawn(ex::starts_on(scheduler, exec::just_from([&, id](auto sink) noexcept {
                                  completed.fetch_add(1, std::memory_order_relaxed);
                                  remember("done-" + std::to_string(id));
                                  return sink();
                                })));
      ++report.spawned;
      remember("spawn-" + std::to_string(id));
    }

    auto first = scope.spawn_future(
        ex::starts_on(scheduler, exec::just_from([](auto sink) noexcept { return sink(10); })));
    auto second = scope.spawn_future(
        ex::starts_on(scheduler, exec::just_from([](auto sink) noexcept { return sink(20); })));
    auto failed = scope.spawn_future(ex::starts_on(
        scheduler, exec::just_from([](auto sink) { return sink(failing_result{}); })));

    if (auto value = ex::sync_wait(std::move(first)))
      report.future_sum += std::get<0>(*value);
    if (auto value = ex::sync_wait(std::move(second)))
      report.future_sum += std::get<0>(*value);
    try {
      (void)ex::sync_wait(std::move(failed));
    } catch (const std::exception &err) {
      report.error_message = err.what();
    }
    ex::sync_wait(scope.on_empty());
  } catch (...) {
    ex::sync_wait(scope.on_empty());
    throw;
  }

  report.completed = completed.load(std::memory_order_relaxed);
  report.destroyed_after_empty = report.completed == report.spawned;
  return report;
}

} // namespace c10_c2_10