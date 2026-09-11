#pragma once
#include <concepts>
#include <string>
#include <utility>

namespace c10_e3 {
void tag_invoke();
struct receiver {
  std::string label{"rx"};
};
struct operation_state {
  int value{};
  std::string path;
};
struct connect_t;
struct member_sender {
  int value{};
  operation_state connect(receiver) const { return {value, "member"}; }
  friend operation_state tag_invoke(const connect_t &, member_sender self, receiver);
};
struct legacy_sender {
  int value{};
  friend operation_state tag_invoke(const connect_t &, legacy_sender self, receiver);
};
struct no_connect_sender {};
template <class S, class R>
concept member_connectable = requires(S sender, R rx) { sender.connect(rx); };
template <class S, class R>
concept legacy_connectable =
    requires(const connect_t &tag, S sender, R rx) { tag_invoke(tag, sender, rx); };
struct connect_t {
  template <class S, class R>
    requires member_connectable<S, R> || legacy_connectable<S, R>
  operation_state operator()(S sender, R rx) const {
    if constexpr (member_connectable<S, R>)
      return sender.connect(rx);
    else
      return tag_invoke(*this, sender, rx);
  }
};
inline constexpr connect_t connect{};
inline operation_state tag_invoke(const connect_t &, member_sender self, receiver) {
  return {self.value, "tag_invoke"};
}
inline operation_state tag_invoke(const connect_t &, legacy_sender self, receiver) {
  return {self.value, "tag_invoke"};
}
inline std::string lookup_report() {
  return "member -> tag_invoke; raw ADL is not in the current protocol";
}
} // namespace c10_e3
