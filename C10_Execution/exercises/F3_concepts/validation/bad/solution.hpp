#pragma once
#include <exception>
#include <string>
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
template <class R, class Sigs>
concept my_receiver_of = my_receiver<R>;
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
