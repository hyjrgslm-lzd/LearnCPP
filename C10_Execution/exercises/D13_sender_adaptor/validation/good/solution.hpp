#pragma once
#include <stdexec/execution.hpp>

#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

namespace c10_d13 {

namespace ex = stdexec;

template <class... Lists> struct merge_lists;
template <> struct merge_lists<> {
  using type = ex::completion_signatures<>;
};
template <class... Sigs> struct merge_lists<ex::completion_signatures<Sigs...>> {
  using type = ex::completion_signatures<Sigs...>;
};
template <class... A, class... B, class... Rest>
struct merge_lists<ex::completion_signatures<A...>, ex::completion_signatures<B...>, Rest...>
    : merge_lists<ex::completion_signatures<A..., B...>, Rest...> {};
template <class... Lists> using merge_lists_t = typename merge_lists<Lists...>::type;

template <class Sig> struct keep_one {
  using type = ex::completion_signatures<Sig>;
};

template <class Completions> struct with_tap_error;
template <class... Sig> struct with_tap_error<ex::completion_signatures<Sig...>> {
  using type = merge_lists_t<typename keep_one<Sig>::type...,
                             ex::completion_signatures<ex::set_error_t(std::exception_ptr)>>;
};

template <class Receiver, class Function> struct observing_receiver {
  using receiver_concept = ex::receiver_tag;
  Receiver out;
  Function fn;

  template <class... Values> void set_value(Values &&...values) && noexcept {
    try {
      std::invoke(fn, values...);
      ex::set_value(std::move(out), std::forward<Values>(values)...);
    } catch (...) {
      ex::set_error(std::move(out), std::current_exception());
    }
  }
  template <class Error> void set_error(Error &&error) && noexcept {
    ex::set_error(std::move(out), std::forward<Error>(error));
  }
  void set_stopped() && noexcept { ex::set_stopped(std::move(out)); }
  auto get_env() const noexcept -> decltype(ex::get_env(out)) { return ex::get_env(out); }
};

template <class Sender, class Receiver, class Function> struct observing_state {
  using operation_state_concept = ex::operation_state_tag;
  using wrapped_receiver = observing_receiver<Receiver, Function>;
  using inner_state = ex::connect_result_t<Sender, wrapped_receiver>;
  inner_state inner;
  observing_state(Sender &&sender, Receiver &&receiver, Function &&fn)
      : inner(ex::connect(
            std::forward<Sender>(sender),
            wrapped_receiver{std::forward<Receiver>(receiver), std::forward<Function>(fn)})) {}
  observing_state(const observing_state &) = delete;
  observing_state(observing_state &&) = delete;
  void start() & noexcept { ex::start(inner); }
};

template <class Sender, class Function> struct tap_sender {
  using sender_concept = ex::sender_tag;
  Sender source;
  Function fn;

  template <class Self, class... Env>
  static consteval auto get_completion_signatures() ->
      typename with_tap_error<ex::completion_signatures_of_t<Sender, Env...>>::type {
    return {};
  }

  template <class Receiver> auto connect(Receiver receiver) && {
    using clean_receiver = std::remove_cvref_t<Receiver>;
    return observing_state<Sender, clean_receiver, Function>{std::move(source), std::move(receiver),
                                                             std::move(fn)};
  }
};

template <class Sender, class Function> auto tap(Sender &&sender, Function function) {
  return tap_sender<std::remove_cvref_t<Sender>, std::remove_cvref_t<Function>>{
      std::forward<Sender>(sender), std::move(function)};
}

} // namespace c10_d13