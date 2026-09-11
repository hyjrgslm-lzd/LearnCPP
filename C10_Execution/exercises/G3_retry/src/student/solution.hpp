#pragma once

#include <c10/test.hpp>
#include <stdexec/execution.hpp>

#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace c10_g3 {

namespace ex = stdexec;

template <class Receiver> struct unfinished_op {
  using operation_state_concept = ex::operation_state_tag;
  Receiver receiver_;
  explicit unfinished_op(Receiver receiver) : receiver_(std::move(receiver)) {}
  unfinished_op(const unfinished_op &) = delete;
  unfinished_op(unfinished_op &&) = delete;
  void start() & noexcept {
    ex::set_error(std::move(receiver_), std::make_exception_ptr(c10::unfinished(
                                            "G3 retry: implement bounded retry sender")));
  }
};

template <class Factory> struct retry_sender {
  using sender_concept = ex::sender_tag;
  Factory factory_;
  int max_attempts_;
  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }
  template <class Receiver> auto connect(Receiver receiver) && {
    return unfinished_op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

template <class Factory>
auto retry(Factory factory, int max_attempts) -> retry_sender<std::remove_cvref_t<Factory>> {
  if (max_attempts <= 0) {
    throw std::invalid_argument("max_attempts must be positive");
  }
  throw c10::unfinished("G3 retry: implement bounded retry sender");
  return retry_sender<std::remove_cvref_t<Factory>>{std::move(factory), max_attempts};
}

} // namespace c10_g3
