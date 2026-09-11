#pragma once

#include <stdexec/execution.hpp>

#include <type_traits>
#include <utility>

namespace c10_g1 {

namespace ex = stdexec;

template <class InnerSender> struct immovable_operation_sender {
  using sender_concept = ex::sender_tag;

  InnerSender inner_;

  template <class Self, class... Env>
  static consteval auto get_completion_signatures()
      -> ex::completion_signatures_of_t<InnerSender, Env...> {
    return {};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    using inner_op_t = ex::connect_result_t<InnerSender, Receiver>;

    inner_op_t inner_op_;

    op(InnerSender &&sender, Receiver &&receiver)
        : inner_op_(ex::connect(std::move(sender), std::move(receiver))) {}
    op(const op &) = delete;
    op(op &&) = delete;
    auto operator=(const op &) -> op & = delete;
    auto operator=(op &&) -> op & = delete;

    void start() & noexcept { ex::start(inner_op_); }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    using receiver_t = std::remove_cvref_t<Receiver>;
    return op<receiver_t>{std::move(inner_), std::move(receiver)};
  }

  auto get_env() const noexcept -> decltype(ex::get_env(inner_)) { return ex::get_env(inner_); }
};

template <class Sender, class F> auto my_then(Sender &&sender, F f) {
  auto oracle = stdexec::then(std::forward<Sender>(sender), std::move(f));
  return immovable_operation_sender<decltype(oracle)>{std::move(oracle)};
}

} // namespace c10_g1
