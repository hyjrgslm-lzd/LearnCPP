#pragma once
#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <stdexec/execution.hpp>

namespace c10_h3 {
namespace ex = stdexec;

struct stopped : std::runtime_error {
  stopped() : std::runtime_error("stopped") {}
};

template <class T> class task {
public:
  using sender_concept = ex::sender_tag;
  struct promise_type {
    task get_return_object() { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept {}
    template <class U> void return_value(U &&) noexcept {}
    template <class Sender> auto await_transform(Sender &&) {
      struct awaiter {
        bool await_ready() noexcept { return true; }
        void await_suspend(std::coroutine_handle<>) noexcept {}
        T await_resume() { return T{}; }
      };
      return awaiter{};
    }
  };
  auto operator co_await() && {
    struct awaiter {
      bool await_ready() noexcept { return true; }
      void await_suspend(std::coroutine_handle<>) noexcept {}
      T await_resume() { return T{}; }
    };
    return awaiter{};
  }
  T sync_wait() && { return T{}; }
  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(T), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    std::optional<Receiver> receiver;
    explicit op(Receiver r) : receiver(std::move(r)) {}
    void start() & noexcept { ex::set_value(std::move(*receiver), T{}); }
  };
  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

template <> class task<void> {
public:
  using sender_concept = ex::sender_tag;
  struct promise_type {
    task get_return_object() { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept {}
    void return_void() noexcept {}
    template <class Sender> auto await_transform(Sender &&) {
      struct awaiter {
        bool await_ready() noexcept { return true; }
        void await_suspend(std::coroutine_handle<>) noexcept {}
        void await_resume() noexcept {}
      };
      return awaiter{};
    }
  };
  auto operator co_await() && {
    struct awaiter {
      bool await_ready() noexcept { return true; }
      void await_suspend(std::coroutine_handle<>) noexcept {}
      void await_resume() noexcept {}
    };
    return awaiter{};
  }
  void sync_wait() && {}
  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    std::optional<Receiver> receiver;
    explicit op(Receiver r) : receiver(std::move(r)) {}
    void start() & noexcept { ex::set_value(std::move(*receiver)); }
  };
  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};
} // namespace c10_h3
