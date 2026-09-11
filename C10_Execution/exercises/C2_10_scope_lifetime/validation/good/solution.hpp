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
  scope_report out;
  std::atomic<int> done{0};
  std::mutex log_mutex;
  exec::static_thread_pool pool{2};
  exec::async_scope scope;

  auto log = [&](std::string event) {
    std::lock_guard guard(log_mutex);
    out.events.emplace_back(std::move(event));
  };

  try {
    for (int n : {1, 2, 3}) {
      log("spawn-" + std::to_string(n));
      scope.spawn(ex::starts_on(pool.get_scheduler(), exec::just_from([&, n](auto sink) noexcept {
                                  done.fetch_add(1, std::memory_order_relaxed);
                                  log("done-" + std::to_string(n));
                                  return sink();
                                })));
      ++out.spawned;
    }

    auto ten = scope.spawn_future(ex::starts_on(
        pool.get_scheduler(), exec::just_from([](auto sink) noexcept { return sink(10); })));
    auto twenty = scope.spawn_future(ex::starts_on(
        pool.get_scheduler(), exec::just_from([](auto sink) noexcept { return sink(20); })));
    auto broken = scope.spawn_future(ex::starts_on(
        pool.get_scheduler(), exec::just_from([](auto sink) { return sink(failing_result{}); })));

    auto ten_value = ex::sync_wait(std::move(ten));
    auto twenty_value = ex::sync_wait(std::move(twenty));
    if (ten_value)
      out.future_sum += std::get<0>(*ten_value);
    if (twenty_value)
      out.future_sum += std::get<0>(*twenty_value);
    try {
      (void)ex::sync_wait(std::move(broken));
    } catch (const std::exception &error) {
      out.error_message = error.what();
    }

    ex::sync_wait(scope.on_empty());
  } catch (...) {
    ex::sync_wait(scope.on_empty());
    throw;
  }

  out.completed = done.load(std::memory_order_relaxed);
  out.destroyed_after_empty = out.completed == out.spawned;
  return out;
}

} // namespace c10_c2_10