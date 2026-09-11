#pragma once

#include <condition_variable>
#include <concepts>
#include <coroutine>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <exec/any_sender_of.hpp>
#include <stdexec/execution.hpp>

namespace c10_h3 {
namespace ex = stdexec;

using any_scheduler_completions_t =
    ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr),
                              ex::set_stopped_t()>;
using any_scheduler_receiver_t = experimental::execution::any_receiver<any_scheduler_completions_t>;
using any_scheduler_sender_t = experimental::execution::any_sender<any_scheduler_receiver_t>;
using any_scheduler_t = experimental::execution::any_scheduler<any_scheduler_sender_t>;

struct stopped : std::runtime_error {
  stopped() : std::runtime_error("stopped") {}
};

struct stop_unwind {};

template <class> inline constexpr bool dependent_false_v = false;

template <class Operation> struct operation_slot {
  alignas(Operation) unsigned char storage[sizeof(Operation)];
  bool engaged = false;
  operation_slot() = default;
  operation_slot(const operation_slot &) = delete;
  operation_slot(operation_slot &&) = delete;
  ~operation_slot() { reset(); }
  template <class Make> void emplace_from(Make make) {
    reset();
    ::new (static_cast<void *>(storage)) Operation(make());
    engaged = true;
  }
  void reset() noexcept {
    if (engaged) {
      ptr()->~Operation();
      engaged = false;
    }
  }
  auto ptr() noexcept -> Operation * { return reinterpret_cast<Operation *>(storage); }
};

template <class Values> struct single_value;

template <> struct single_value<std::variant<std::tuple<>>> {
  using type = void;
};

template <class T> struct single_value<std::variant<std::tuple<T>>> {
  using type = T;
};

template <class Sender, class Env>
using sender_value_t =
    typename single_value<ex::value_types_of_t<Sender, Env, std::tuple, std::variant>>::type;

template <class Promise, class Sender, class Result> struct sender_awaiter;

template <class Promise, class Sender> struct sender_awaiter<Promise, Sender, void> {
  using handle_t = std::coroutine_handle<Promise>;
  Sender sender;
  std::variant<std::monostate, std::monostate, std::exception_ptr, stopped> result;
  Promise *promise = nullptr;

  struct receiver {
    using receiver_concept = ex::receiver_tag;
    sender_awaiter *self;
    handle_t coroutine;
    void set_value() && noexcept {
      self->result.template emplace<1>();
      coroutine.resume();
    }
    void set_error(std::exception_ptr e) && noexcept {
      self->result.template emplace<2>(e);
      coroutine.resume();
    }
    void set_stopped() && noexcept {
      self->result.template emplace<3>();
      coroutine.resume();
    }
    auto get_env() const noexcept -> decltype(std::declval<Promise const &>().get_env()) {
      return self->promise->get_env();
    }
  };

  using op_t = decltype(ex::connect(std::declval<Sender>(), std::declval<receiver>()));
  operation_slot<op_t> op;

  bool await_ready() noexcept { return false; }
  void await_suspend(handle_t h) {
    promise = &h.promise();
    op.emplace_from([&] { return ex::connect(std::move(sender), receiver{this, h}); });
    ex::start(*op.ptr());
  }
  void await_resume() {
    switch (result.index()) {
    case 1:
      return;
    case 2:
      std::rethrow_exception(std::get<2>(result));
    case 3:
      promise->was_stopped = true;
      throw stop_unwind{};
    default:
      throw std::logic_error("sender did not complete");
    }
  }
};

template <class Promise, class Sender, class Result> struct sender_awaiter {
  using handle_t = std::coroutine_handle<Promise>;
  Sender sender;
  std::variant<std::monostate, Result, std::exception_ptr, stopped> result;
  Promise *promise = nullptr;

  struct receiver {
    using receiver_concept = ex::receiver_tag;
    sender_awaiter *self;
    handle_t coroutine;
    void set_value(Result v) && noexcept {
      try {
        self->result.template emplace<1>(std::move(v));
      } catch (...) {
        self->result.template emplace<2>(std::current_exception());
      }
      coroutine.resume();
    }
    void set_error(std::exception_ptr e) && noexcept {
      self->result.template emplace<2>(e);
      coroutine.resume();
    }
    void set_stopped() && noexcept {
      self->result.template emplace<3>();
      coroutine.resume();
    }
    auto get_env() const noexcept -> decltype(std::declval<Promise const &>().get_env()) {
      return self->promise->get_env();
    }
  };

  using op_t = decltype(ex::connect(std::declval<Sender>(), std::declval<receiver>()));
  operation_slot<op_t> op;

  bool await_ready() noexcept { return false; }
  void await_suspend(handle_t h) {
    promise = &h.promise();
    op.emplace_from([&] { return ex::connect(std::move(sender), receiver{this, h}); });
    ex::start(*op.ptr());
  }
  Result await_resume() {
    switch (result.index()) {
    case 1:
      return std::move(std::get<1>(result));
    case 2:
      std::rethrow_exception(std::get<2>(result));
    case 3:
      promise->was_stopped = true;
      throw stop_unwind{};
    default:
      throw std::logic_error("sender did not complete");
    }
  }
};

template <class T> class task {
public:
  struct promise_type;
  using handle_t = std::coroutine_handle<promise_type>;

private:
  handle_t handle_ = {};

public:
  using sender_concept = ex::sender_tag;

  explicit task(handle_t handle) noexcept : handle_(handle) {}
  task(task &&other) noexcept : handle_(std::exchange(other.handle_, {})) {}
  task(const task &) = delete;
  task &operator=(const task &) = delete;
  task &operator=(task &&other) noexcept {
    if (this != &other) {
      if (handle_)
        handle_.destroy();
      handle_ = std::exchange(other.handle_, {});
    }
    return *this;
  }
  ~task() {
    if (handle_)
      handle_.destroy();
  }

  struct task_awaiter {
    task child;
    bool await_ready() noexcept { return false; }
    template <class ParentPromise>
    auto await_suspend(std::coroutine_handle<ParentPromise> parent) noexcept
        -> std::coroutine_handle<> {
      if constexpr (requires { parent.promise().get_env(); }) {
        child.handle_.promise().copy_env_from(parent.promise().get_env());
      }
      child.handle_.promise().continuation = parent;
      return child.handle_;
    }
    T await_resume() {
      auto &promise = child.handle_.promise();
      if (promise.error)
        std::rethrow_exception(promise.error);
      if (promise.was_stopped)
        throw stop_unwind{};
      return std::move(*promise.value);
    }
  };

  auto operator co_await() && { return task_awaiter{std::move(*this)}; }

  T sync_wait() && {
    auto &promise = handle_.promise();
    promise.use_default_env();
    handle_.resume();
    std::unique_lock lock(promise.mutex);
    promise.cv.wait(lock, [&] { return promise.done; });
    if (promise.error)
      std::rethrow_exception(promise.error);
    if (promise.was_stopped)
      throw stopped{};
    return std::move(*promise.value);
  }

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(T), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct sender_op {
    using operation_state_concept = ex::operation_state_tag;
    handle_t child;
    std::optional<Receiver> receiver;
    sender_op(task t, Receiver r) : child(std::exchange(t.handle_, {})), receiver(std::move(r)) {}
    ~sender_op() {
      if (child)
        child.destroy();
    }
    sender_op(const sender_op &) = delete;
    sender_op(sender_op &&) = delete;

    static void complete_receiver(void *state, handle_t h) noexcept {
      static_cast<sender_op *>(state)->complete(h);
    }

    void complete(handle_t h) noexcept {
      auto &promise = h.promise();
      try {
        if (promise.error) {
          ex::set_error(std::move(*receiver), promise.error);
        } else if (promise.was_stopped) {
          ex::set_stopped(std::move(*receiver));
        } else {
          ex::set_value(std::move(*receiver), std::move(*promise.value));
        }
      } catch (...) {
        ex::set_error(std::move(*receiver), std::current_exception());
      }
    }

    void start() & noexcept {
      try {
        auto &promise = child.promise();
        auto env = ex::get_env(*receiver);
        promise.copy_env_from(env);
        if (promise.stop_requested()) {
          ex::set_stopped(std::move(*receiver));
          return;
        }
        promise.completion_state = this;
        promise.completion = complete_receiver;
        child.resume();
      } catch (...) {
        ex::set_error(std::move(*receiver), std::current_exception());
      }
    }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    return sender_op<std::remove_cvref_t<Receiver>>{std::move(*this), std::move(receiver)};
  }

  struct promise_type {
    std::mutex mutex;
    std::condition_variable cv;
    std::optional<T> value;
    std::exception_ptr error;
    bool was_stopped = false;
    bool done = false;
    std::coroutine_handle<> continuation = {};
    ex::inplace_stop_source stop_source;
    ex::inplace_stop_token stop_token = stop_source.get_token();
    any_scheduler_t scheduler = ex::inline_scheduler{};
    void *completion_state = nullptr;
    void (*completion)(void *, handle_t) noexcept = nullptr;

    task get_return_object() { return task{handle_t::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    auto get_env() const noexcept {
      return ex::env{ex::prop{ex::get_stop_token, stop_token},
                     ex::prop{ex::get_scheduler, scheduler}};
    }
    void use_default_env() {
      stop_token = stop_source.get_token();
      scheduler = ex::inline_scheduler{};
    }
    bool stop_requested() const noexcept { return stop_token.stop_requested(); }
    template <class Env> void copy_env_from(const Env &env) {
      if constexpr (requires { ex::get_stop_token(env); }) {
        auto token = ex::get_stop_token(env);
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(token)>,
                                     ex::inplace_stop_token>) {
          stop_token = token;
        } else if constexpr (ex::unstoppable_token<std::remove_cvref_t<decltype(token)>>) {
          stop_token = stop_source.get_token();
        } else {
          static_assert(dependent_false_v<decltype(token)>,
                        "H3 teaching task forwards stdexec::inplace_stop_token; other stop token "
                        "types need an explicit callback bridge");
        }
      }
      if constexpr (requires { ex::get_scheduler(env); }) {
        using scheduler_t = std::remove_cvref_t<decltype(ex::get_scheduler(env))>;
        static_assert(
            std::constructible_from<any_scheduler_t, scheduler_t>,
            "H3 teaching task requires a scheduler convertible to its type-erased scheduler");
        scheduler = any_scheduler_t{ex::get_scheduler(env)};
      } else {
        scheduler = ex::inline_scheduler{};
      }
    }
    struct final_awaiter {
      bool await_ready() noexcept { return false; }
      auto await_suspend(handle_t h) noexcept -> std::coroutine_handle<> {
        auto &promise = h.promise();
        auto next_continuation = promise.continuation;
        auto terminal_callback = promise.completion;
        auto terminal_state = promise.completion_state;
        {
          std::lock_guard lock(promise.mutex);
          promise.done = true;
          promise.cv.notify_all();
        }
        if (terminal_callback) {
          terminal_callback(terminal_state, h);
          return std::noop_coroutine();
        }
        return next_continuation ? next_continuation : std::noop_coroutine();
      }
      void await_resume() noexcept {}
    };
    final_awaiter final_suspend() noexcept { return {}; }
    template <class U> void return_value(U &&v) { value.emplace(std::forward<U>(v)); }
    void unhandled_exception() noexcept {
      try {
        throw;
      } catch (const stop_unwind &) {
        was_stopped = true;
      } catch (...) {
        error = std::current_exception();
      }
    }
    template <class Sender> auto await_transform(Sender &&sender) {
      using sender_t = std::remove_cvref_t<Sender>;
      using env_t = decltype(std::declval<promise_type const &>().get_env());
      using result_t = sender_value_t<sender_t, env_t>;
      return sender_awaiter<promise_type, sender_t, result_t>{std::forward<Sender>(sender)};
    }
    template <class U> auto await_transform(task<U> &&child) noexcept {
      return std::move(child).operator co_await();
    }
  };
};

template <> class task<void> {
public:
  struct promise_type;
  using handle_t = std::coroutine_handle<promise_type>;

private:
  handle_t handle_ = {};

public:
  using sender_concept = ex::sender_tag;
  explicit task(handle_t handle) noexcept : handle_(handle) {}
  task(task &&other) noexcept : handle_(std::exchange(other.handle_, {})) {}
  task(const task &) = delete;
  task &operator=(const task &) = delete;
  ~task() {
    if (handle_)
      handle_.destroy();
  }

  struct task_awaiter {
    handle_t child;
    task_awaiter(handle_t h) noexcept : child(h) {}
    task_awaiter(const task_awaiter &) = delete;
    task_awaiter(task_awaiter &&other) noexcept : child(std::exchange(other.child, {})) {}
    ~task_awaiter() {
      if (child)
        child.destroy();
    }
    bool await_ready() noexcept { return false; }
    template <class ParentPromise>
    auto await_suspend(std::coroutine_handle<ParentPromise> parent) noexcept
        -> std::coroutine_handle<> {
      if constexpr (requires { parent.promise().get_env(); }) {
        child.promise().copy_env_from(parent.promise().get_env());
      }
      child.promise().continuation = parent;
      return child;
    }
    void await_resume() {
      auto &promise = child.promise();
      if (promise.error)
        std::rethrow_exception(promise.error);
      if (promise.was_stopped)
        throw stop_unwind{};
    }
  };

  auto operator co_await() && { return task_awaiter{std::exchange(handle_, {})}; }

  void sync_wait() && {
    auto &promise = handle_.promise();
    promise.use_default_env();
    handle_.resume();
    std::unique_lock lock(promise.mutex);
    promise.cv.wait(lock, [&] { return promise.done; });
    if (promise.error)
      std::rethrow_exception(promise.error);
    if (promise.was_stopped)
      throw stopped{};
  }

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct sender_op {
    using operation_state_concept = ex::operation_state_tag;
    handle_t child;
    std::optional<Receiver> receiver;
    sender_op(task t, Receiver r) : child(std::exchange(t.handle_, {})), receiver(std::move(r)) {}
    ~sender_op() {
      if (child)
        child.destroy();
    }
    sender_op(const sender_op &) = delete;
    sender_op(sender_op &&) = delete;

    static void complete_receiver(void *state, handle_t h) noexcept {
      static_cast<sender_op *>(state)->complete(h);
    }

    void complete(handle_t h) noexcept {
      auto &promise = h.promise();
      try {
        if (promise.error) {
          ex::set_error(std::move(*receiver), promise.error);
        } else if (promise.was_stopped) {
          ex::set_stopped(std::move(*receiver));
        } else {
          ex::set_value(std::move(*receiver));
        }
      } catch (...) {
        ex::set_error(std::move(*receiver), std::current_exception());
      }
    }

    void start() & noexcept {
      try {
        auto &promise = child.promise();
        auto env = ex::get_env(*receiver);
        promise.copy_env_from(env);
        if (promise.stop_requested()) {
          ex::set_stopped(std::move(*receiver));
          return;
        }
        promise.completion_state = this;
        promise.completion = complete_receiver;
        child.resume();
      } catch (...) {
        ex::set_error(std::move(*receiver), std::current_exception());
      }
    }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    return sender_op<std::remove_cvref_t<Receiver>>{std::move(*this), std::move(receiver)};
  }

  struct promise_type {
    std::mutex mutex;
    std::condition_variable cv;
    std::exception_ptr error;
    bool was_stopped = false;
    bool done = false;
    std::coroutine_handle<> continuation = {};
    ex::inplace_stop_source stop_source;
    ex::inplace_stop_token stop_token = stop_source.get_token();
    any_scheduler_t scheduler = ex::inline_scheduler{};
    void *completion_state = nullptr;
    void (*completion)(void *, handle_t) noexcept = nullptr;
    task get_return_object() { return task{handle_t::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    auto get_env() const noexcept {
      return ex::env{ex::prop{ex::get_stop_token, stop_token},
                     ex::prop{ex::get_scheduler, scheduler}};
    }
    void use_default_env() {
      stop_token = stop_source.get_token();
      scheduler = ex::inline_scheduler{};
    }
    bool stop_requested() const noexcept { return stop_token.stop_requested(); }
    template <class Env> void copy_env_from(const Env &env) {
      if constexpr (requires { ex::get_stop_token(env); }) {
        auto token = ex::get_stop_token(env);
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(token)>,
                                     ex::inplace_stop_token>) {
          stop_token = token;
        } else if constexpr (ex::unstoppable_token<std::remove_cvref_t<decltype(token)>>) {
          stop_token = stop_source.get_token();
        } else {
          static_assert(dependent_false_v<decltype(token)>,
                        "H3 teaching task forwards stdexec::inplace_stop_token; other stop token "
                        "types need an explicit callback bridge");
        }
      }
      if constexpr (requires { ex::get_scheduler(env); }) {
        using scheduler_t = std::remove_cvref_t<decltype(ex::get_scheduler(env))>;
        static_assert(
            std::constructible_from<any_scheduler_t, scheduler_t>,
            "H3 teaching task requires a scheduler convertible to its type-erased scheduler");
        scheduler = any_scheduler_t{ex::get_scheduler(env)};
      } else {
        scheduler = ex::inline_scheduler{};
      }
    }
    struct final_awaiter {
      bool await_ready() noexcept { return false; }
      auto await_suspend(handle_t h) noexcept -> std::coroutine_handle<> {
        auto &promise = h.promise();
        auto next_continuation = promise.continuation;
        auto terminal_callback = promise.completion;
        auto terminal_state = promise.completion_state;
        {
          std::lock_guard lock(promise.mutex);
          promise.done = true;
          promise.cv.notify_all();
        }
        if (terminal_callback) {
          terminal_callback(terminal_state, h);
          return std::noop_coroutine();
        }
        return next_continuation ? next_continuation : std::noop_coroutine();
      }
      void await_resume() noexcept {}
    };
    final_awaiter final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {
      try {
        throw;
      } catch (const stop_unwind &) {
        was_stopped = true;
      } catch (...) {
        error = std::current_exception();
      }
    }
    template <class Sender> auto await_transform(Sender &&sender) {
      using sender_t = std::remove_cvref_t<Sender>;
      using env_t = decltype(std::declval<promise_type const &>().get_env());
      using result_t = sender_value_t<sender_t, env_t>;
      return sender_awaiter<promise_type, sender_t, result_t>{std::forward<Sender>(sender)};
    }
    template <class U> auto await_transform(task<U> &&child) noexcept {
      return std::move(child).operator co_await();
    }
  };
};

} // namespace c10_h3
