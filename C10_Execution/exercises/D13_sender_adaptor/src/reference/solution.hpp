#pragma once
#include <stdexec/execution.hpp>

#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

namespace c10_d13 {

namespace ex = stdexec;

template <class... Completions> struct concat_completions;
template <> struct concat_completions<> {
  using type = ex::completion_signatures<>;
};
template <class... Sigs> struct concat_completions<ex::completion_signatures<Sigs...>> {
  using type = ex::completion_signatures<Sigs...>;
};
template <class... A, class... B, class... Rest>
struct concat_completions<ex::completion_signatures<A...>, ex::completion_signatures<B...>, Rest...>
    : concat_completions<ex::completion_signatures<A..., B...>, Rest...> {};
template <class... C> using concat_completions_t = typename concat_completions<C...>::type;

template <class Signature> struct tap_signature {
  using type = ex::completion_signatures<Signature>;
};
template <class Completions> struct tap_completions;
template <class... Sigs> struct tap_completions<ex::completion_signatures<Sigs...>> {
  using type = concat_completions_t<typename tap_signature<Sigs>::type...,
                                    ex::completion_signatures<ex::set_error_t(std::exception_ptr)>>;
};

template <class DownstreamReceiver, class F> struct tap_receiver {
  using receiver_concept = ex::receiver_tag;
  DownstreamReceiver downstream;
  F f;

  template <class... Values> void set_value(Values &&...values) && noexcept {
    try {
      std::invoke(f, values...);
      ex::set_value(std::move(downstream), std::forward<Values>(values)...);
    } catch (...) {
      ex::set_error(std::move(downstream), std::current_exception());
    }
  }

  template <class Error> void set_error(Error &&error) && noexcept {
    ex::set_error(std::move(downstream), std::forward<Error>(error));
  }

  void set_stopped() && noexcept { ex::set_stopped(std::move(downstream)); }

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
  static consteval auto get_completion_signatures() ->
      typename tap_completions<ex::completion_signatures_of_t<InnerSender, Env...>>::type {
    return {};
  }

  template <class Receiver> auto connect(Receiver receiver) && {
    using receiver_t = std::remove_cvref_t<Receiver>;
    return tap_operation_state<InnerSender, receiver_t, F>{std::move(inner), std::move(receiver),
                                                           std::move(f)};
  }
};

template <class Sender, class F> auto tap(Sender &&sender, F f) {
  return tap_sender<std::remove_cvref_t<Sender>, std::remove_cvref_t<F>>{
      std::forward<Sender>(sender), std::move(f)};
}

} // namespace c10_d13
