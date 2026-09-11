#pragma once
#include <concepts>
#include <exception>
#include <string>
#include <type_traits>

namespace c10_f3 {

struct set_value_t;
struct set_error_t;
struct set_stopped_t;

template <class... Sigs> struct completion_signatures {};

template <class S>
concept my_sender = requires {
  typename S::sender_concept;
  typename S::completion_signatures;
};

template <class R>
concept my_receiver = requires(R &receiver) {
  typename R::receiver_concept;
  receiver.set_stopped();
};

template <class R, class Sig> struct signature_ok : std::false_type {};

template <class R, class... Args>
    struct signature_ok<R, set_value_t(Args...)> : std::bool_constant <
                                                   requires(R &receiver, Args... args) {
  receiver.set_value(args...);
}>{};

template <class R, class Error>
    struct signature_ok<R, set_error_t(Error)> : std::bool_constant <
                                                 requires(R &receiver, Error error) {
  receiver.set_error(error);
}>{};

template <class R>
    struct signature_ok<R, set_stopped_t()> : std::bool_constant < requires(R &receiver) {
  receiver.set_stopped();
}>{};

template <class R, class Sigs> struct signatures_ok;

template <class R, class... Sigs>
struct signatures_ok<R, completion_signatures<Sigs...>>
    : std::bool_constant<(signature_ok<R, Sigs>::value && ...)> {};

template <class R, class Sigs>
concept my_receiver_of = my_receiver<R> && signatures_ok<R, Sigs>::value;

template <class S, class R>
concept my_sender_to = my_sender<S> && my_receiver_of<R, typename S::completion_signatures>;

struct valid_sender {
  using sender_concept = void;
  using completion_signatures =
      c10_f3::completion_signatures<set_value_t(int), set_error_t(std::exception_ptr),
                                    set_stopped_t()>;
};

struct invalid_sender {
  using sender_concept = void;
};

struct valid_receiver {
  using receiver_concept = void;
  void set_value(int) {}
  void set_error(std::exception_ptr) {}
  void set_stopped() {}
};

struct missing_receiver_tag {
  void set_stopped() {}
};

struct partial_receiver {
  using receiver_concept = void;
  void set_value(std::string) {}
  void set_error(std::exception_ptr) {}
  void set_stopped() {}
};

} // namespace c10_f3
