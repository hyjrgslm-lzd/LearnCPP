#pragma once
#include <c10/test.hpp>
#include <concepts>
#include <string>

namespace c10_e3 {
struct receiver {
  std::string label{"rx"};
};
struct operation_state {
  int value{};
  std::string path;
};
struct member_sender {
  int value{};
  operation_state connect(receiver) const {
    throw c10::unfinished("E3 evolution: implement member connect");
  }
};
struct legacy_sender {
  int value{};
};
struct no_connect_sender {};
struct connect_t;
template <class S, class R>
concept member_connectable = requires(S sender, R rx) { sender.connect(rx); };
template <class S, class R>
concept legacy_connectable = std::same_as<S, legacy_sender> && std::same_as<R, receiver>;
struct connect_t {
  template <class S, class R> operation_state operator()(S &&, R &&) const {
    throw c10::unfinished("E3 evolution: implement member-first connect CPO");
  }
};
inline constexpr connect_t connect{};
inline std::string lookup_report() { return "unfinished"; }
} // namespace c10_e3
