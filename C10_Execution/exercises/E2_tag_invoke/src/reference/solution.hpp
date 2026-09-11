#pragma once
#include <concepts>
#include <string>
#include <utility>

namespace c10_e2 {
namespace tag_invoke_detail {
void tag_invoke();
}
using tag_invoke_detail::tag_invoke;
template <class Tag, class... Args>
concept tag_invocable =
    requires(Tag tag, Args &&...args) { tag_invoke(tag, std::forward<Args>(args)...); };
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
  friend operation_state tag_invoke(connect_t, sender self, receiver rx);
};
struct scheduler {
  std::string name;
};
struct env {
  scheduler sched;
  friend scheduler tag_invoke(get_scheduler_t, const env &self);
};
struct connect_t {
  template <class Sender, class Receiver>
    requires tag_invocable<connect_t, Sender, Receiver>
  decltype(auto) operator()(Sender &&sender, Receiver &&receiver) const {
    return tag_invoke(*this, std::forward<Sender>(sender), std::forward<Receiver>(receiver));
  }
};
struct start_t {
  template <class Op>
    requires tag_invocable<start_t, Op &>
  decltype(auto) operator()(Op &op) const {
    return tag_invoke(*this, op);
  }
};
struct get_scheduler_t {
  template <class Env>
    requires tag_invocable<get_scheduler_t, const Env &>
  decltype(auto) operator()(const Env &env) const {
    return tag_invoke(*this, env);
  }
};
inline constexpr connect_t connect{};
inline constexpr start_t start{};
inline constexpr get_scheduler_t get_scheduler{};
inline operation_state tag_invoke(connect_t, sender self, receiver rx) {
  return {self.value, rx.label, false};
}
inline void tag_invoke(start_t, operation_state &op) { op.started = true; }
inline scheduler tag_invoke(get_scheduler_t, const env &self) { return self.sched; }
} // namespace c10_e2
