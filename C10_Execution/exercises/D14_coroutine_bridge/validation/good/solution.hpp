#pragma once

#include <stdexec/execution.hpp>

#include <string>
#include <thread>

namespace c10_d14 {
namespace ex = stdexec;

inline auto sender_graph(int input) {
  return ex::let_value(ex::just(input),
                       [](int value) { return ex::just(std::to_string(value + value)); });
}

inline ex::task<std::string> coroutine_task(int input) {
  auto doubled = co_await ex::just(input + input);
  auto text = co_await ex::just(std::to_string(doubled));
  co_return text;
}

inline auto stopped_source() { return ex::just_stopped(); }

inline auto sender_stopped_status() {
  return ex::let_stopped(stopped_source(), [] { return ex::just(std::string{"stopped"}); });
}

inline ex::task<std::string> coroutine_stopped_status() {
  auto status = co_await sender_stopped_status();
  co_return status;
}

template <class Scheduler> inline auto sender_switch_thread(Scheduler scheduler) {
  return ex::schedule(scheduler) | ex::then([] { return std::this_thread::get_id(); });
}

template <class Scheduler>
inline ex::task<std::thread::id> coroutine_switch_thread(Scheduler scheduler) {
  auto id = co_await sender_switch_thread(scheduler);
  co_return id;
}

inline ex::task<int> stdexec_task() {
  auto two = co_await ex::just(2);
  co_return two + 3;
}

} // namespace c10_d14
