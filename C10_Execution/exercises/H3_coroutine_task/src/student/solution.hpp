#pragma once
#include <c10/test.hpp>
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
    task get_return_object() {
      return task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept {}
    template <class U> void return_value(U &&) noexcept {}
    template <class Sender> auto await_transform(Sender &&) {
      struct awaiter {
        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<>) {
          throw c10::unfinished("H3 task: implement sender await bridge");
        }
        T await_resume() { throw c10::unfinished("H3 task: implement await_resume"); }
      };
      return awaiter{};
    }
  };
  // Provided frame ownership keeps the deliberately unfinished lazy starter leak-free.
  using handle_t = std::coroutine_handle<promise_type>;
  handle_t handle_{};
  task() = default;
  explicit task(handle_t h) : handle_(h) {}
  task(task &&other) noexcept : handle_(std::exchange(other.handle_, {})) {}
  task(const task &) = delete;
  ~task() {
    if (handle_)
      handle_.destroy();
  }
  auto operator co_await() && {
    struct awaiter {
      bool await_ready() noexcept { return false; }
      void await_suspend(std::coroutine_handle<>) {
        throw c10::unfinished("H3 task: implement task continuation");
      }
      T await_resume() { throw c10::unfinished("H3 task: implement nested await_resume"); }
    };
    return awaiter{};
  }
  T sync_wait() && { throw c10::unfinished("H3 task: implement sync_wait"); }
  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(T), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    std::optional<Receiver> receiver;
    explicit op(Receiver r) : receiver(std::move(r)) {}
    void start() & noexcept {
      ex::set_error(std::move(*receiver),
                    std::make_exception_ptr(c10::unfinished("H3 task: implement task sender")));
    }
  };
  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

template <> class task<void> {
public:
  using sender_concept = ex::sender_tag;
  struct promise_type {
    task get_return_object() {
      return task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept {}
    void return_void() noexcept {}
    template <class Sender> auto await_transform(Sender &&) {
      struct awaiter {
        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<>) {
          throw c10::unfinished("H3 task: implement void sender await bridge");
        }
        void await_resume() { throw c10::unfinished("H3 task: implement void await_resume"); }
      };
      return awaiter{};
    }
  };
  // Provided frame ownership keeps the deliberately unfinished lazy starter leak-free.
  using handle_t = std::coroutine_handle<promise_type>;
  handle_t handle_{};
  task() = default;
  explicit task(handle_t h) : handle_(h) {}
  task(task &&other) noexcept : handle_(std::exchange(other.handle_, {})) {}
  task(const task &) = delete;
  ~task() {
    if (handle_)
      handle_.destroy();
  }
  auto operator co_await() && {
    struct awaiter {
      bool await_ready() noexcept { return false; }
      void await_suspend(std::coroutine_handle<>) {
        throw c10::unfinished("H3 task: implement void task continuation");
      }
      void await_resume() { throw c10::unfinished("H3 task: implement void nested await_resume"); }
    };
    return awaiter{};
  }
  void sync_wait() && { throw c10::unfinished("H3 task: implement void sync_wait"); }
  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    std::optional<Receiver> receiver;
    explicit op(Receiver r) : receiver(std::move(r)) {}
    void start() & noexcept {
      ex::set_error(std::move(*receiver), std::make_exception_ptr(c10::unfinished(
                                              "H3 task: implement void task sender")));
    }
  };
  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};
} // namespace c10_h3
