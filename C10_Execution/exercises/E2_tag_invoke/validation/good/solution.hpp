#pragma once
#include <concepts>
#include <string>
#include <utility>

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
struct scheduler {
  std::string name;
};
struct env {
  scheduler sched;
};
template <class Tag, class... Args>
concept tag_invocable =
    requires(Tag tag, Args &&...args) { tag_invoke(tag, std::forward<Args>(args)...); };
struct connect_t {
  template <class Sender, class Receiver>
    requires tag_invocable<connect_t, Sender, Receiver>
  auto operator()(Sender &&sender, Receiver &&receiver) const {
    return tag_invoke(connect_t{}, std::forward<Sender>(sender), std::forward<Receiver>(receiver));
  }
};
struct start_t {
  template <class Op>
    requires tag_invocable<start_t, Op &>
  void operator()(Op &op) const {
    tag_invoke(start_t{}, op);
  }
};
struct get_scheduler_t {
  template <class Env>
    requires tag_invocable<get_scheduler_t, const Env &>
  auto operator()(const Env &env) const {
    return tag_invoke(get_scheduler_t{}, env);
  }
};
struct sender {
  int value{};
  friend operation_state tag_invoke(connect_t, sender self, receiver rx) {
    return {self.value, rx.label, false};
  }
};
inline void tag_invoke(start_t, operation_state &op) { op.started = true; }
inline scheduler tag_invoke(get_scheduler_t, const env &self) { return self.sched; }
inline constexpr connect_t connect{};
inline constexpr start_t start{};
inline constexpr get_scheduler_t get_scheduler{};
} // namespace c10_e2
