#pragma once
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

#include <optional>
#include <string>
#include <thread>

namespace c10_d14 {
namespace ex = stdexec;

inline auto sender_graph(int input) {
  return ex::just(input) | ex::then([](int value) { return value * 2; }) |
         ex::then([](int value) { return std::to_string(value); });
}

inline ex::task<std::string> coroutine_task(int input) {
  int value = co_await ex::just(input);
  co_return std::to_string(value * 2);
}

inline auto stopped_source() {
  return ex::just(0) | ex::let_value([](int) { return ex::just_stopped(); });
}

inline auto sender_stopped_status() {
  return ex::let_stopped(stopped_source(), [] { return ex::just(std::string{"stopped"}); });
}

inline ex::task<std::string> coroutine_stopped_status() {
  std::string value =
      co_await ex::let_stopped(stopped_source(), [] { return ex::just(std::string{"stopped"}); });
  co_return value;
}

template <class Scheduler> inline auto sender_switch_thread(Scheduler scheduler) {
  return ex::starts_on(scheduler, ex::just() | ex::then([] { return std::this_thread::get_id(); }));
}

template <class Scheduler>
inline ex::task<std::thread::id> coroutine_switch_thread(Scheduler scheduler) {
  std::thread::id id = co_await sender_switch_thread(scheduler);
  co_return id;
}

inline ex::task<int> stdexec_task() { co_return 5; }
} // namespace c10_d14
