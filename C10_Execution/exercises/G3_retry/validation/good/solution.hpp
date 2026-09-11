#pragma once

#include <exec/repeat_until.hpp>
#include <stdexec/execution.hpp>

#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace c10_g3 {

namespace ex = stdexec;

template <class Factory> struct retry_state {
  Factory factory;
  int max_attempts = 0;
  int attempts = 0;
  std::optional<int> value;
  std::exception_ptr last_error;
};

template <class State> auto make_retry_graph(std::shared_ptr<State> state) {
  auto attempt_once =
      ex::upon_error(ex::let_value(ex::just(),
                                   [state] {
                                     ++state->attempts;
                                     return ex::then(state->factory(), [state](int value) noexcept {
                                       state->value = value;
                                       return true;
                                     });
                                   }),
                     [state](std::exception_ptr error) noexcept {
                       state->last_error = std::move(error);
                       return state->attempts >= state->max_attempts;
                     });

  return ex::let_value(exec::repeat_until(std::move(attempt_once)), [state] {
    return ex::then(ex::just(), [state] -> int {
      if (state->value) {
        return *state->value;
      }
      std::rethrow_exception(state->last_error
                                 ? state->last_error
                                 : std::make_exception_ptr(std::runtime_error("retry exhausted")));
    });
  });
}

template <class Factory> struct retry_sender {
  using sender_concept = ex::sender_tag;
  Factory factory_;
  int max_attempts_;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> auto connect(Receiver receiver) && {
    auto state = std::make_shared<retry_state<Factory>>(
        retry_state<Factory>{std::move(factory_), max_attempts_});
    return ex::connect(make_retry_graph(state), std::move(receiver));
  }
};

template <class Factory> auto retry(Factory factory, int max_attempts) {
  if (max_attempts <= 0) {
    throw std::invalid_argument("max_attempts must be positive");
  }
  return retry_sender<std::remove_cvref_t<Factory>>{std::move(factory), max_attempts};
}

} // namespace c10_g3
