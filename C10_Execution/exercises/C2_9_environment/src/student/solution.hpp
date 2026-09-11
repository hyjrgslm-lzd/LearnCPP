#pragma once
#include <c10/test.hpp>
#include <stdexec/execution.hpp>
#include <string>
#include <thread>
#include <utility>

namespace c10_c2_9 {
struct get_task_id_t {
  static consteval bool query(stdexec::forwarding_query_t) noexcept { return true; }
  template <class Env>
    requires requires(const Env &env, get_task_id_t q) { env.query(q); }
  decltype(auto) operator()(const Env &env) const noexcept(noexcept(env.query(*this))) {
    return env.query(*this);
  }
};
inline constexpr get_task_id_t get_task_id{};

struct runtime_context_snapshot {
  std::string payload;
  int task_id{};
  bool stop_requested{};
  std::thread::id start_thread{};
  std::thread::id followup_thread{};
};
template <class Payload> auto read_runtime_context(Payload &&payload) {
  return stdexec::just(std::string{std::forward<Payload>(payload)}) |
         stdexec::then([](std::string) -> runtime_context_snapshot {
           throw c10::unfinished("C2_9: implement environment-reading sender");
         });
}
} // namespace c10_c2_9
