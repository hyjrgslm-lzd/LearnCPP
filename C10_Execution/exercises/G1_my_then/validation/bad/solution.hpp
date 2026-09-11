#pragma once

#include <stdexec/execution.hpp>

#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

namespace c10_g1 {

namespace ex = stdexec;

template <class Result> struct my_then_result_completion {
  using type = ex::completion_signatures<ex::set_value_t(Result)>;
};

template <> struct my_then_result_completion<void> {
  using type = ex::completion_signatures<ex::set_value_t()>;
};

template <class F, class... Args>
struct my_then_value_completion : my_then_result_completion<std::invoke_result_t<F, Args...>> {};

template <class... Completions> struct concat_completions;

template <> struct concat_completions<> {
  using type = ex::completion_signatures<>;
};

template <class... Left> struct concat_completions<ex::completion_signatures<Left...>> {
  using type = ex::completion_signatures<Left...>;
};

template <class... Left, class... Right, class... Rest>
struct concat_completions<ex::completion_signatures<Left...>, ex::completion_signatures<Right...>,
                          Rest...>
    : concat_completions<ex::completion_signatures<Left..., Right...>, Rest...> {};

template <class... Completions>
using concat_completions_t = typename concat_completions<Completions...>::type;

template <class F, class Signature> struct transform_signature;

template <class F, class... Args>
struct transform_signature<F, ex::set_value_t(Args...)> : my_then_value_completion<F, Args...> {};

template <class F, class Error> struct transform_signature<F, ex::set_error_t(Error)> {
  using type = ex::completion_signatures<ex::set_error_t(Error)>;
};

template <class F> struct transform_signature<F, ex::set_stopped_t()> {
  using type = ex::completion_signatures<ex::set_stopped_t()>;
};

template <class F, class ChildCompletions> struct my_then_completions;

template <class F, class... Signatures>
struct my_then_completions<F, ex::completion_signatures<Signatures...>> {
  using type = concat_completions_t<typename transform_signature<F, Signatures>::type...,
                                    ex::completion_signatures<ex::set_error_t(std::exception_ptr)>>;
};

template <class DownstreamReceiver, class F> struct my_then_receiver {
  using receiver_concept = ex::receiver_tag;

  DownstreamReceiver downstream_;
  F f_;

  template <class... Values> void set_value(Values &&...values) && noexcept {
    try {
      if constexpr (std::is_void_v<std::invoke_result_t<F, Values...>>) {
        std::invoke(std::move(f_), std::forward<Values>(values)...);
        ex::set_value(std::move(downstream_));
      } else {
        ex::set_value(std::move(downstream_),
                      std::invoke(std::move(f_), std::forward<Values>(values)...));
      }
    } catch (...) {
      ex::set_error(std::move(downstream_), std::current_exception());
    }
  }

  template <class Error> void set_error(Error &&) && noexcept {
    ex::set_value(std::move(downstream_), -404);
  }

  void set_stopped() && noexcept { ex::set_stopped(std::move(downstream_)); }

  auto get_env() const noexcept -> decltype(ex::get_env(downstream_)) {
    return ex::get_env(downstream_);
  }
};

template <class InnerSender, class DownstreamReceiver, class F> struct my_then_operation_state {
  using operation_state_concept = ex::operation_state_tag;
  using inner_receiver_t = my_then_receiver<DownstreamReceiver, F>;
  using inner_op_t = ex::connect_result_t<InnerSender, inner_receiver_t>;

  inner_op_t inner_op_;

  my_then_operation_state(InnerSender &&sender, DownstreamReceiver &&receiver, F &&f)
      : inner_op_(ex::connect(
            std::forward<InnerSender>(sender),
            inner_receiver_t{std::forward<DownstreamReceiver>(receiver), std::forward<F>(f)})) {}

  my_then_operation_state(const my_then_operation_state &) = delete;
  my_then_operation_state(my_then_operation_state &&) = delete;
  auto operator=(const my_then_operation_state &) -> my_then_operation_state & = delete;
  auto operator=(my_then_operation_state &&) -> my_then_operation_state & = delete;

  void start() & noexcept { ex::start(inner_op_); }
};

template <class InnerSender, class F> struct my_then_sender {
  using sender_concept = ex::sender_tag;

  InnerSender inner_;
  F f_;

  template <class Self, class... Env>
  static consteval auto get_completion_signatures() ->
      typename my_then_completions<F, ex::completion_signatures_of_t<InnerSender, Env...>>::type {
    return {};
  }

  template <class Receiver> auto connect(Receiver receiver) && {
    using receiver_t = std::remove_cvref_t<Receiver>;
    return my_then_operation_state<InnerSender, receiver_t, F>{std::move(inner_),
                                                               std::move(receiver), std::move(f_)};
  }

  auto get_env() const noexcept -> decltype(ex::get_env(inner_)) { return ex::get_env(inner_); }
};

template <class Sender, class F> auto my_then(Sender &&sender, F f) {
  return my_then_sender<std::remove_cvref_t<Sender>, std::remove_cvref_t<F>>{
      std::forward<Sender>(sender), std::move(f)};
}

} // namespace c10_g1
