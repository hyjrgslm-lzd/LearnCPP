#pragma once
#include <c10/test.hpp>
#include <stdexec/execution.hpp>

#include <string>
#include <thread>
#include <optional>

namespace c10_d14 {
namespace ex = stdexec;

inline auto sender_graph(int input) {
  return ex::just(input) |
         ex::then([](int) -> int { throw c10::unfinished("D14 coroutine bridge: sender graph"); }) |
         ex::then([](int value) { return std::to_string(value); });
}

inline ex::task<std::string> coroutine_task(int) {
  throw c10::unfinished("D14 coroutine bridge: coroutine task");
  co_return std::string{};
}

inline auto sender_stopped_status() { return ex::just(std::string{"unfinished"}); }

inline ex::task<std::string> coroutine_stopped_status() {
  throw c10::unfinished("D14 coroutine bridge: stopped mapping");
  co_return std::string{};
}

template <class Scheduler> inline auto sender_switch_thread(Scheduler) {
  return ex::just(std::this_thread::get_id());
}

template <class Scheduler> inline ex::task<std::thread::id> coroutine_switch_thread(Scheduler) {
  throw c10::unfinished("D14 coroutine bridge: scheduler switch");
  co_return std::thread::id{};
}

inline ex::task<int> stdexec_task() {
  throw c10::unfinished("D14 coroutine bridge: stdexec task sender");
  co_return 0;
}
} // namespace c10_d14
