#pragma once
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

#include <optional>
#include <string>
#include <thread>

namespace c10_d14 {
namespace ex = stdexec;

inline auto sender_graph(int) { return ex::just(std::string{"42"}); }
inline ex::task<std::string> coroutine_task(int) { co_return std::string{"41"}; }
inline auto sender_stopped_status() { return ex::just(std::string{"value"}); }
inline ex::task<std::string> coroutine_stopped_status() { co_return std::string{"value"}; }
template <class Scheduler> inline auto sender_switch_thread(Scheduler) {
  return ex::just(std::this_thread::get_id());
}
template <class Scheduler> inline ex::task<std::thread::id> coroutine_switch_thread(Scheduler) {
  co_return std::this_thread::get_id();
}
inline ex::task<int> stdexec_task() { co_return 5; }
} // namespace c10_d14
