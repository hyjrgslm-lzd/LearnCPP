#pragma once

#include <c10/test.hpp>
#include <stdexec/execution.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace c10_g1 {

namespace ex = stdexec;

template <class Sender, class F> struct unfinished_then_sender {
  using sender_concept = ex::sender_tag;

  Sender sender_;
  F f_;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver_;

    explicit op(Receiver receiver) : receiver_(std::move(receiver)) {}
    op(const op &) = delete;
    op(op &&) = delete;

    void start() & noexcept {
      ex::set_error(std::move(receiver_), std::make_exception_ptr(c10::unfinished(
                                              "G1 my_then: implement sender adaptor")));
    }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

template <class Sender, class F> auto my_then(Sender &&sender, F f) {
  return unfinished_then_sender<std::remove_cvref_t<Sender>, std::remove_cvref_t<F>>{
      std::forward<Sender>(sender), std::move(f)};
}

} // namespace c10_g1
