#pragma once
#include <stdexec/execution.hpp>

#include <functional>
#include <type_traits>
#include <utility>

namespace c10_d13 {

namespace ex = stdexec;

template <class DownstreamReceiver, class F> struct tap_receiver {
  using receiver_concept = ex::receiver_tag;
  DownstreamReceiver downstream;
  F f;

  template <class... Values> void set_value(Values &&...values) && noexcept {
    std::invoke(f, values...);
    ex::set_value(std::move(downstream), std::forward<Values>(values)...);
  }
  template <class Error> void set_error(Error &&error) && noexcept {
    ex::set_error(std::move(downstream), std::forward<Error>(error));
  }
  void set_stopped() && noexcept { ex::set_value(std::move(downstream), 0); }
  auto get_env() const noexcept -> decltype(ex::get_env(downstream)) {
    return ex::get_env(downstream);
  }
};

template <class InnerSender, class DownstreamReceiver, class F> struct tap_operation_state {
  using operation_state_concept = ex::operation_state_tag;
  using receiver_t = tap_receiver<DownstreamReceiver, F>;
  using inner_op_t = ex::connect_result_t<InnerSender, receiver_t>;
  inner_op_t inner_op;
  tap_operation_state(InnerSender &&sender, DownstreamReceiver &&receiver, F &&f)
      : inner_op(ex::connect(
            std::forward<InnerSender>(sender),
            receiver_t{std::forward<DownstreamReceiver>(receiver), std::forward<F>(f)})) {}
  tap_operation_state(const tap_operation_state &) = delete;
  tap_operation_state(tap_operation_state &&) = delete;
  void start() & noexcept { ex::start(inner_op); }
};

template <class InnerSender, class F> struct tap_sender {
  using sender_concept = ex::sender_tag;
  InnerSender inner;
  F f;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures()
      -> ex::completion_signatures_of_t<InnerSender, Env...> {
    return {};
  }
  template <class Receiver> auto connect(Receiver receiver) && {
    return tap_operation_state<InnerSender, std::remove_cvref_t<Receiver>, F>{
        std::move(inner), std::move(receiver), std::move(f)};
  }
};

template <class Sender, class F> auto tap(Sender &&sender, F f) {
  return tap_sender<std::remove_cvref_t<Sender>, std::remove_cvref_t<F>>{
      std::forward<Sender>(sender), std::move(f)};
}

} // namespace c10_d13
