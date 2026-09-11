#pragma once

#include <stdexec/execution.hpp>

#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace c10_g3 {

namespace ex = stdexec;

template <class Factory, class Receiver> struct retry_op;

template <class Factory, class Receiver> struct recursive_receiver {
  using receiver_concept = ex::receiver_tag;
  retry_op<Factory, Receiver> *owner;

  void set_value(int value) && noexcept;
  void set_error(std::exception_ptr error) && noexcept;
  void set_stopped() && noexcept;
  auto get_env() const noexcept -> decltype(ex::get_env(std::declval<Receiver const &>()));
};

template <class Factory, class Receiver> struct retry_op {
  using operation_state_concept = ex::operation_state_tag;
  using sender_t = decltype(std::declval<Factory &>()());

  Factory factory;
  Receiver receiver;
  int max_attempts = 0;
  int attempts = 0;
  std::exception_ptr last_error;
  bool done = false;

  retry_op(Factory f, Receiver r, int limit)
      : factory(std::move(f)), receiver(std::move(r)), max_attempts(limit) {}
  retry_op(const retry_op &) = delete;
  retry_op(retry_op &&) = delete;

  void start() & noexcept { start_one(); }

  void start_one() noexcept {
    if (done)
      return;
    if (attempts >= max_attempts) {
      done = true;
      ex::set_error(std::move(receiver),
                    last_error ? last_error
                               : std::make_exception_ptr(std::runtime_error("retry exhausted")));
      return;
    }
    ++attempts;
    try {
      auto sender = factory();
      auto op = ex::connect(std::move(sender), recursive_receiver<Factory, Receiver>{this});
      ex::start(op);
    } catch (...) {
      last_error = std::current_exception();
      start_one();
    }
  }
};

template <class Factory, class Receiver>
void recursive_receiver<Factory, Receiver>::set_value(int value) && noexcept {
  owner->done = true;
  ex::set_value(std::move(owner->receiver), value);
}

template <class Factory, class Receiver>
void recursive_receiver<Factory, Receiver>::set_error(std::exception_ptr error) && noexcept {
  thread_local int depth = 0;
  ++depth;
  owner->last_error = std::move(error);
  if (depth > 64) {
    owner->done = true;
    ex::set_error(std::move(owner->receiver),
                  std::make_exception_ptr(std::runtime_error("recursive retry budget exceeded")));
  } else {
    owner->start_one();
  }
  --depth;
}

template <class Factory, class Receiver>
void recursive_receiver<Factory, Receiver>::set_stopped() && noexcept {
  owner->done = true;
  ex::set_stopped(std::move(owner->receiver));
}

template <class Factory, class Receiver>
auto recursive_receiver<Factory, Receiver>::get_env() const noexcept
    -> decltype(ex::get_env(std::declval<Receiver const &>())) {
  return ex::get_env(owner->receiver);
}

template <class Factory> struct retry_sender {
  using sender_concept = ex::sender_tag;
  Factory factory;
  int max_attempts = 0;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> auto connect(Receiver receiver) && {
    return retry_op<Factory, std::remove_cvref_t<Receiver>>{std::move(factory), std::move(receiver),
                                                            max_attempts};
  }
};

template <class Factory> auto retry(Factory factory, int max_attempts) {
  if (max_attempts <= 0) {
    throw std::invalid_argument("max_attempts must be positive");
  }
  return retry_sender<std::remove_cvref_t<Factory>>{std::move(factory), max_attempts};
}

} // namespace c10_g3
