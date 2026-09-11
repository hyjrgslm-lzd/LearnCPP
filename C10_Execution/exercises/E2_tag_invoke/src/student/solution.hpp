#pragma once
#include <c10/test.hpp>
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
template <> struct tag_invocable_impl<start_t, operation_state> : std::true_type {};
template <> struct tag_invocable_impl<get_scheduler_t, env> : std::true_type {};
template <class Tag, class... Args>
concept tag_invocable = tag_invocable_impl<Tag, std::remove_cvref_t<Args>...>::value;
struct connect_t {
  template <class Sender, class Receiver>
    requires tag_invocable<connect_t, Sender, Receiver>
  operation_state operator()(Sender &&, Receiver &&) const {
    throw c10::unfinished("E2 tag_invoke: implement connect_t");
  }
};
struct start_t {
  template <class Op>
    requires tag_invocable<start_t, Op &>
  void operator()(Op &) const {
    throw c10::unfinished("E2 tag_invoke: implement start_t");
  }
};
struct get_scheduler_t {
  template <class Env>
    requires tag_invocable<get_scheduler_t, const Env &>
  scheduler operator()(const Env &) const {
    throw c10::unfinished("E2 tag_invoke: implement get_scheduler_t");
  }
};
inline constexpr connect_t connect{};
inline constexpr start_t start{};
inline constexpr get_scheduler_t get_scheduler{};
} // namespace c10_e2
