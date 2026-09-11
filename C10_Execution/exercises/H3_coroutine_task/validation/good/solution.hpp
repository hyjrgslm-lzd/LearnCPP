#pragma once
#include <coroutine>
#include <exception>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <exec/task.hpp>
#include <stdexec/execution.hpp>

namespace c10_h3 {
namespace ex = stdexec;
struct stopped : std::runtime_error {
  stopped() : std::runtime_error("stopped") {}
};

// A value wrapper keeps exception_ptr-as-value distinct from the engine's error alternative.
template <class T> struct value_box {
  T value;
  value_box(T v) : value(std::move(v)) {}
};
template <class T> using payload_t = std::conditional_t<std::is_void_v<T>, void, value_box<T>>;

// exec::basic_task's documented context extension uses get_start_scheduler.
// This exercise also exposes that capability through get_scheduler.
template <class T> struct task_context : exec::default_task_context<T> {
  using base = exec::default_task_context<T>;
  using base::base;
  using base::query;
  template <class Promise> using promise_context_t = task_context;
  decltype(auto) query(ex::get_scheduler_t) const noexcept {
    return base::query(ex::get_start_scheduler);
  }
};

template <class T> class task {
  using engine = exec::basic_task<payload_t<T>, task_context<payload_t<T>>>;
  engine inner_;
  template <class Awaiter> struct unwrap_awaiter {
    Awaiter inner;
    bool await_ready() noexcept { return inner.await_ready(); }
    template <class P> auto await_suspend(std::coroutine_handle<P> h) noexcept {
      return inner.await_suspend(h);
    }
    T await_resume() {
      if constexpr (std::is_void_v<T>)
        inner.await_resume();
      else
        return std::move(inner.await_resume().value);
    }
  };

public:
  using sender_concept = ex::sender_tag;
  using promise_type = typename engine::promise_type;
  task(engine value) : inner_(std::move(value)) {}
  task(task &&) = default;
  task(const task &) = delete;
  template <class P> auto as_awaitable(P &parent) && {
    using awaiter = decltype(std::move(inner_).as_awaitable(parent));
    return unwrap_awaiter<awaiter>{std::move(inner_).as_awaitable(parent)};
  }
  auto sync_wait() && {
    ex::inplace_stop_source source;
    auto result = ex::sync_wait(ex::write_env(
        std::move(inner_), ex::env{ex::prop{ex::get_stop_token, source.get_token()}}));
    if (!result)
      throw stopped{};
    if constexpr (!std::is_void_v<T>)
      return std::move(std::get<0>(*result).value);
  }
  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    if constexpr (std::is_void_v<T>)
      return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr),
                                       ex::set_stopped_t()>{};
    else
      return ex::completion_signatures<ex::set_value_t(T), ex::set_error_t(std::exception_ptr),
                                       ex::set_stopped_t()>{};
  }
  template <class Receiver> struct operation {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    struct bridge {
      using receiver_concept = ex::receiver_tag;
      Receiver *downstream;
      template <class... Values> void set_value(Values &&...values) && noexcept {
        if constexpr (std::is_void_v<T>)
          ex::set_value(std::move(*downstream));
        else
          ex::set_value(std::move(*downstream), std::move(values.value)...);
      }
      void set_error(std::exception_ptr error) && noexcept {
        ex::set_error(std::move(*downstream), error);
      }
      void set_stopped() && noexcept { ex::set_stopped(std::move(*downstream)); }
      auto get_env() const noexcept {
        auto env = ex::get_env(*downstream);
        if constexpr (requires { ex::get_scheduler(env); })
          return ex::env{ex::prop{ex::get_start_scheduler, ex::get_scheduler(env)}, std::move(env)};
        else
          return ex::env{ex::prop{ex::get_start_scheduler, ex::inline_scheduler{}}, std::move(env)};
      }
    };
    using inner_op = decltype(ex::connect(std::declval<engine>(), std::declval<bridge>()));
    inner_op inner;
    operation(engine child, Receiver out)
        : receiver(std::move(out)), inner(ex::connect(std::move(child), bridge{&receiver})) {}
    operation(const operation &) = delete;
    operation(operation &&) = delete;
    void start() & noexcept {
      if (ex::get_stop_token(ex::get_env(receiver)).stop_requested()) {
        ex::set_stopped(std::move(receiver));
        return;
      }
      ex::start(inner);
    }
  };
  template <class Receiver> auto connect(Receiver receiver) && {
    return operation<Receiver>{std::move(inner_), std::move(receiver)};
  }
};
} // namespace c10_h3
