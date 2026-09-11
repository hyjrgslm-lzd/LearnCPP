#pragma once
#include <c10/test.hpp>
#include <stdexec/execution.hpp>

namespace c10_d13 {
struct never_started {
  using operation_state_concept = stdexec::operation_state_tag;
  never_started() = default;
  never_started(const never_started &) = delete;
  never_started(never_started &&) = delete;
  void start() & noexcept {}
};
struct unfinished_sender {
  using sender_concept = stdexec::sender_tag;
  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return stdexec::completion_signatures<>{};
  }
  template <class Receiver> auto connect(Receiver &&) const -> never_started {
    throw c10::unfinished("implement tap sender adaptor");
    return never_started{};
  }
};
template <class Sender, class F> auto tap(Sender &&, F &&) { return unfinished_sender{}; }
} // namespace c10_d13
