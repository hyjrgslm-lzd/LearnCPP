#pragma once
#include <concepts>
#include <string>
#include <type_traits>
namespace c10_e2 {
void tag_invoke();
struct receiver {
  std::string label{"rx"};
};
struct operation_state {
  int value{};
  std::string receiver_label;
  bool started{};
};
struct connect_t;
struct start_t;
struct get_scheduler_t;
struct sender {
  int value{};
};
struct scheduler {
  std::string name;
};
struct env {
  scheduler sched;
};
template <class Tag, class... Args> struct tag_invocable_impl : std::false_type {};
template <> struct tag_invocable_impl<connect_t, sender, receiver> : std::true_type {};
template <> struct tag_invocable_impl<start_t, operation_state &> : std::true_type {};
template <> struct tag_invocable_impl<get_scheduler_t, const env &> : std::true_type {};
template <class Tag, class... Args>
concept tag_invocable = tag_invocable_impl<Tag, Args...>::value;
struct connect_t {
  template <class Sender, class Receiver> auto operator()(Sender &&, Receiver &&) const {
    return operation_state{0, "fallback", false};
  }
};
struct start_t {
  void operator()(operation_state &op) const { op.started = true; }
};
struct get_scheduler_t {
  template <class Env> auto operator()(const Env &env) const { return env.sched; }
};
inline constexpr connect_t connect{};
inline constexpr start_t start{};
inline constexpr get_scheduler_t get_scheduler{};
} // namespace c10_e2
