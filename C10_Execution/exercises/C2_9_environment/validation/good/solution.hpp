#pragma once
#include <stdexec/execution.hpp>

#include <string>
#include <thread>
#include <utility>

namespace c10_c2_9 {
namespace ex = stdexec;
struct get_task_id_t {
  static consteval bool query(ex::forwarding_query_t) noexcept { return true; }
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
  std::string owned{std::forward<Payload>(payload)};
  auto caller = std::this_thread::get_id();
  auto id = ex::read_env(get_task_id);
  auto stopped = ex::read_env(ex::get_stop_token) |
                 ex::then([](auto token) { return token.stop_requested(); });
  auto follow = ex::read_env(ex::get_scheduler) | ex::let_value([](auto sch) {
                  return ex::write_env(
                      ex::schedule(sch) | ex::then([] { return std::this_thread::get_id(); }),
                      ex::env{ex::prop{ex::get_stop_token, ex::never_stop_token{}}});
                });
  return ex::when_all(ex::just(std::move(owned)), std::move(id), std::move(stopped),
                      ex::just(caller), std::move(follow)) |
         ex::then([](std::string text, int task_id, bool stop, std::thread::id start,
                     std::thread::id followup) {
           return runtime_context_snapshot{std::move(text), task_id, stop, start, followup};
         });
}
} // namespace c10_c2_9
