#pragma once

#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <stdexec/execution.hpp>

namespace c10_p2::mini {

struct empty_env {};

struct connect_t {
  template <class Sender, class Receiver>
  auto operator()(Sender &&sender, Receiver &&receiver) const
      noexcept(noexcept(std::forward<Sender>(sender).connect(std::forward<Receiver>(receiver)))) {
    return std::forward<Sender>(sender).connect(std::forward<Receiver>(receiver));
  }
};
struct start_t {
  template <class Op> void operator()(Op &op) const noexcept(noexcept(op.start())) { op.start(); }
};
struct set_value_t {
  template <class Receiver, class... Values>
  void operator()(Receiver &&receiver, Values &&...values) const noexcept(
      noexcept(std::forward<Receiver>(receiver).set_value(std::forward<Values>(values)...))) {
    std::forward<Receiver>(receiver).set_value(std::forward<Values>(values)...);
  }
};
struct set_error_t {
  template <class Receiver, class Error>
  void operator()(Receiver &&receiver, Error &&error) const
      noexcept(noexcept(std::forward<Receiver>(receiver).set_error(std::forward<Error>(error)))) {
    std::forward<Receiver>(receiver).set_error(std::forward<Error>(error));
  }
};
struct set_stopped_t {
  template <class Receiver>
  void operator()(Receiver &&receiver) const
      noexcept(noexcept(std::forward<Receiver>(receiver).set_stopped())) {
    std::forward<Receiver>(receiver).set_stopped();
  }
};
struct get_env_t {
  template <class Receiver> auto operator()(const Receiver &receiver) const noexcept {
    if constexpr (requires { receiver.get_env(); }) {
      return receiver.get_env();
    } else {
      return empty_env{};
    }
  }
};
struct schedule_t {
  template <class Scheduler>
  auto operator()(Scheduler scheduler) const noexcept(noexcept(scheduler.schedule())) {
    return scheduler.schedule();
  }
};
struct get_allocator_t {
  template <class Env> auto operator()(const Env &env) const noexcept {
    if constexpr (requires { env.get_allocator(); }) {
      return env.get_allocator();
    } else {
      return 0;
    }
  }
};

inline constexpr connect_t connect{};
inline constexpr start_t start{};
inline constexpr set_value_t set_value{};
inline constexpr set_error_t set_error{};
inline constexpr set_stopped_t set_stopped{};
inline constexpr get_env_t get_env{};
inline constexpr schedule_t schedule{};
inline constexpr get_allocator_t get_allocator{};

template <class... Signatures> struct completion_signatures {};

template <class Sender, class Env = empty_env>
using completion_signatures_of_t =
    typename std::remove_cvref_t<Sender>::template completion_signatures<Env>;

namespace detail {
struct none {};

template <class...> struct first_value_tuple;

template <> struct first_value_tuple<> {
  using type = std::tuple<>;
};

template <class... Values, class... Rest>
struct first_value_tuple<set_value_t(Values...), Rest...> {
  using type = std::tuple<Values...>;
};

template <class First, class... Rest>
struct first_value_tuple<First, Rest...> : first_value_tuple<Rest...> {};

template <class Completions> struct first_value_from_completions;

template <class... Signatures>
struct first_value_from_completions<completion_signatures<Signatures...>>
    : first_value_tuple<Signatures...> {};

template <class Sig> struct value_alt {
  using type = none;
};

template <class... Values> struct value_alt<set_value_t(Values...)> {
  using type = std::tuple<Values...>;
};

template <class Sig> struct error_alt {
  using type = none;
};

template <class Error> struct error_alt<set_error_t(Error)> {
  using type = Error;
};

template <class... Types> struct keep_not_none;

template <> struct keep_not_none<std::tuple<>> {
  using type = std::variant<std::monostate>;
};

template <class... Kept> struct keep_not_none<std::tuple<Kept...>> {
  using type = std::variant<Kept...>;
};

template <class... Kept, class T, class... Rest>
struct keep_not_none<std::tuple<Kept...>, T, Rest...>
    : keep_not_none<std::tuple<Kept...>, Rest...> {};

template <class... Kept, class T, class... Rest>
  requires(!std::same_as<T, none>)
struct keep_not_none<std::tuple<Kept...>, T, Rest...>
    : keep_not_none<std::tuple<Kept..., T>, Rest...> {};

template <class Completions> struct value_types;

template <class... Signatures>
struct value_types<completion_signatures<Signatures...>>
    : keep_not_none<std::tuple<>, typename value_alt<Signatures>::type...> {};

template <class Completions> struct error_types;

template <class... Signatures>
struct error_types<completion_signatures<Signatures...>>
    : keep_not_none<std::tuple<>, typename error_alt<Signatures>::type...> {};

template <class Result> struct then_value_result {
  using type = completion_signatures<set_value_t(Result)>;
};

template <> struct then_value_result<void> {
  using type = completion_signatures<set_value_t()>;
};

template <class F, class Signature> struct then_signature {
  using type = completion_signatures<Signature>;
};

template <class F, class... Values>
struct then_signature<F, set_value_t(Values...)>
    : then_value_result<std::invoke_result_t<F, Values...>> {};

template <class... Completions> struct concat_completions;

template <> struct concat_completions<> {
  using type = completion_signatures<>;
};

template <class... Signatures> struct concat_completions<completion_signatures<Signatures...>> {
  using type = completion_signatures<Signatures...>;
};

template <class... Left, class... Right, class... Rest>
struct concat_completions<completion_signatures<Left...>, completion_signatures<Right...>, Rest...>
    : concat_completions<completion_signatures<Left..., Right...>, Rest...> {};

template <class F, class Completions> struct then_completions;

template <class F, class... Signatures>
struct then_completions<F, completion_signatures<Signatures...>>
    : concat_completions<typename then_signature<F, Signatures>::type...,
                         completion_signatures<set_error_t(std::exception_ptr)>> {};

template <class Left, class Right> struct tuple_cat_type;

template <class... L, class... R> struct tuple_cat_type<std::tuple<L...>, std::tuple<R...>> {
  using signature = set_value_t(L..., R...);
};

} // namespace detail

template <class Sender, class Env = empty_env>
using value_types_of_t =
    typename detail::value_types<completion_signatures_of_t<Sender, Env>>::type;

template <class Sender, class Env = empty_env>
using error_types_of_t =
    typename detail::error_types<completion_signatures_of_t<Sender, Env>>::type;

template <class Receiver>
concept receiver = requires(Receiver receiver, std::exception_ptr error) {
  mini::set_value(std::move(receiver));
  mini::set_error(std::move(receiver), error);
  mini::set_stopped(std::move(receiver));
};

template <class Sender>
concept sender = requires { typename completion_signatures_of_t<Sender>; };

template <class Op>
concept operation_state = requires(Op &op) { mini::start(op); } && (!std::move_constructible<Op>);

template <class... Values> struct just_sender {
  std::tuple<Values...> values_;

  template <class Env>
  using completion_signatures = mini::completion_signatures<set_value_t(Values...)>;

  template <class Receiver> struct op {
    std::tuple<Values...> values_;
    Receiver receiver_;

    op(std::tuple<Values...> values, Receiver receiver)
        : values_(std::move(values)), receiver_(std::move(receiver)) {}
    op(const op &) = delete;
    op(op &&) = delete;
    auto operator=(const op &) -> op & = delete;
    auto operator=(op &&) -> op & = delete;

    void start() noexcept {
      std::apply(
          [&](auto &&...values) { mini::set_value(std::move(receiver_), std::move(values)...); },
          std::move(values_));
    }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{std::move(values_), std::move(receiver)};
  }
};

template <class... Values> auto just(Values &&...values) {
  return just_sender<std::remove_cvref_t<Values>...>{
      std::tuple<std::remove_cvref_t<Values>...>{std::forward<Values>(values)...}};
}

struct just_error_sender {
  std::exception_ptr error_;

  template <class Env>
  using completion_signatures = mini::completion_signatures<set_error_t(std::exception_ptr)>;

  template <class Receiver> struct op {
    std::exception_ptr error_;
    Receiver receiver_;

    op(std::exception_ptr error, Receiver receiver)
        : error_(std::move(error)), receiver_(std::move(receiver)) {}
    op(const op &) = delete;
    op(op &&) = delete;
    auto operator=(const op &) -> op & = delete;
    auto operator=(op &&) -> op & = delete;

    void start() noexcept { mini::set_error(std::move(receiver_), std::move(error_)); }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{std::move(error_), std::move(receiver)};
  }
};

inline auto just_error(std::exception_ptr error) { return just_error_sender{std::move(error)}; }

struct just_stopped_sender {
  template <class Env> using completion_signatures = mini::completion_signatures<set_stopped_t()>;

  template <class Receiver> struct op {
    Receiver receiver_;

    explicit op(Receiver receiver) : receiver_(std::move(receiver)) {}
    op(const op &) = delete;
    op(op &&) = delete;
    auto operator=(const op &) -> op & = delete;
    auto operator=(op &&) -> op & = delete;

    void start() noexcept { mini::set_stopped(std::move(receiver_)); }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

inline auto just_stopped() { return just_stopped_sender{}; }

template <class Receiver, class F> struct then_receiver {
  Receiver receiver_;
  F function_;

  template <class... Values> void set_value(Values &&...values) noexcept {
    try {
      if constexpr (std::is_void_v<std::invoke_result_t<F, Values...>>) {
        std::invoke(std::move(function_), std::forward<Values>(values)...);
        mini::set_value(std::move(receiver_));
      } else {
        mini::set_value(std::move(receiver_),
                        std::invoke(std::move(function_), std::forward<Values>(values)...));
      }
    } catch (...) {
      mini::set_error(std::move(receiver_), std::current_exception());
    }
  }

  void set_error(std::exception_ptr error) noexcept {
    mini::set_error(std::move(receiver_), std::move(error));
  }

  void set_stopped() noexcept { mini::set_stopped(std::move(receiver_)); }

  auto get_env() const noexcept -> decltype(mini::get_env(receiver_)) {
    return mini::get_env(receiver_);
  }
};

template <class Sender, class F> struct then_sender {
  Sender sender_;
  F function_;

  template <class Env>
  using completion_signatures =
      typename detail::then_completions<F, completion_signatures_of_t<Sender, Env>>::type;

  template <class Receiver> struct op {
    using inner_receiver_t = then_receiver<std::remove_cvref_t<Receiver>, F>;
    using inner_op_t =
        decltype(mini::connect(std::declval<Sender>(), std::declval<inner_receiver_t>()));

    inner_op_t inner_;

    op(Sender sender, Receiver receiver, F function)
        : inner_(mini::connect(std::move(sender),
                               inner_receiver_t{std::move(receiver), std::move(function)})) {}
    op(const op &) = delete;
    op(op &&) = delete;
    auto operator=(const op &) -> op & = delete;
    auto operator=(op &&) -> op & = delete;

    void start() noexcept { mini::start(inner_); }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{std::move(sender_), std::move(receiver),
                                             std::move(function_)};
  }
};

template <class Sender, class F> auto then(Sender &&sender, F function) {
  return then_sender<std::remove_cvref_t<Sender>, std::remove_cvref_t<F>>{
      std::forward<Sender>(sender), std::move(function)};
}

template <class F> struct then_adaptor {
  F function_;

  template <class Sender> auto operator()(Sender &&sender) && {
    return mini::then(std::forward<Sender>(sender), std::move(function_));
  }
};

template <class F> auto then(F function) {
  return then_adaptor<std::remove_cvref_t<F>>{std::move(function)};
}

template <class Sender, class Adaptor>
auto operator|(Sender &&sender, Adaptor &&adaptor)
    -> decltype(std::forward<Adaptor>(adaptor)(std::forward<Sender>(sender))) {
  return std::forward<Adaptor>(adaptor)(std::forward<Sender>(sender));
}

template <class Sender>
using sync_wait_tuple_t =
    typename detail::first_value_from_completions<completion_signatures_of_t<Sender>>::type;

template <class Tuple> struct sync_wait_state {
  std::mutex mutex;
  std::condition_variable cv;
  std::optional<Tuple> value;
  std::exception_ptr error;
  bool done = false;
};

template <class Tuple> struct sync_wait_receiver {
  sync_wait_state<Tuple> *state_;

  template <class... Values> void set_value(Values &&...values) noexcept {
    std::lock_guard lock(state_->mutex);
    state_->value.emplace(std::forward<Values>(values)...);
    state_->done = true;
    state_->cv.notify_one();
  }

  void set_error(std::exception_ptr error) noexcept {
    std::lock_guard lock(state_->mutex);
    state_->error = std::move(error);
    state_->done = true;
    state_->cv.notify_one();
  }

  void set_stopped() noexcept {
    std::lock_guard lock(state_->mutex);
    state_->done = true;
    state_->cv.notify_one();
  }
};

template <class Sender> auto sync_wait(Sender sender) -> std::optional<sync_wait_tuple_t<Sender>> {
  using tuple_t = sync_wait_tuple_t<Sender>;
  sync_wait_state<tuple_t> state;
  auto op = mini::connect(std::move(sender), sync_wait_receiver<tuple_t>{&state});
  mini::start(op);
  {
    std::unique_lock lock(state.mutex);
    state.cv.wait(lock, [&] { return state.done; });
  }
  if (state.error) {
    std::rethrow_exception(state.error);
  }
  return std::move(state.value);
}

template <class Left, class Right> struct when_all_sender {
  Left left_;
  Right right_;

  template <class Env>
  using completion_signatures = mini::completion_signatures<
      typename detail::tuple_cat_type<sync_wait_tuple_t<Left>, sync_wait_tuple_t<Right>>::signature,
      set_error_t(std::exception_ptr), set_stopped_t()>;

  template <class Receiver> struct op {
    using left_tuple_t = sync_wait_tuple_t<Left>;
    using right_tuple_t = sync_wait_tuple_t<Right>;

    struct shared_state {
      Receiver receiver_;
      std::mutex mutex_;
      std::optional<left_tuple_t> left_;
      std::optional<right_tuple_t> right_;
      std::exception_ptr error_;
      bool stopped_ = false;
      int completed_ = 0;

      explicit shared_state(Receiver receiver) : receiver_(std::move(receiver)) {}

      void finish_if_ready() noexcept {
        std::unique_lock lock(mutex_);
        if (++completed_ != 2) {
          return;
        }
        auto error = std::move(error_);
        auto stopped = stopped_;
        auto left = std::move(left_);
        auto right = std::move(right_);
        lock.unlock();

        if (error) {
          mini::set_error(std::move(receiver_), std::move(error));
        } else if (stopped || !left || !right) {
          mini::set_stopped(std::move(receiver_));
        } else {
          std::apply(
              [&](auto &&...lvalues) {
                std::apply(
                    [&](auto &&...rvalues) {
                      mini::set_value(std::move(receiver_), std::move(lvalues)...,
                                      std::move(rvalues)...);
                    },
                    std::move(*right));
              },
              std::move(*left));
        }
      }
    };

    template <bool IsLeft> struct child_receiver {
      shared_state *state_;

      template <class... Values> void set_value(Values &&...values) noexcept {
        {
          std::lock_guard lock(state_->mutex_);
          if constexpr (IsLeft) {
            state_->left_.emplace(std::forward<Values>(values)...);
          } else {
            state_->right_.emplace(std::forward<Values>(values)...);
          }
        }
        state_->finish_if_ready();
      }

      void set_error(std::exception_ptr error) noexcept {
        {
          std::lock_guard lock(state_->mutex_);
          if (!state_->error_) {
            state_->error_ = std::move(error);
          }
        }
        state_->finish_if_ready();
      }

      void set_stopped() noexcept {
        {
          std::lock_guard lock(state_->mutex_);
          state_->stopped_ = true;
        }
        state_->finish_if_ready();
      }
    };

    using left_op_t =
        decltype(mini::connect(std::declval<Left>(), std::declval<child_receiver<true>>()));
    using right_op_t =
        decltype(mini::connect(std::declval<Right>(), std::declval<child_receiver<false>>()));

    shared_state state_;
    left_op_t left_;
    right_op_t right_;

    op(Left left, Right right, Receiver receiver)
        : state_(std::move(receiver)),
          left_(mini::connect(std::move(left), child_receiver<true>{&state_})),
          right_(mini::connect(std::move(right), child_receiver<false>{&state_})) {}
    op(const op &) = delete;
    op(op &&) = delete;
    auto operator=(const op &) -> op & = delete;
    auto operator=(op &&) -> op & = delete;

    void start() noexcept {
      mini::start(left_);
      mini::start(right_);
    }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{std::move(left_), std::move(right_),
                                             std::move(receiver)};
  }
};

template <class Left, class Right> auto when_all(Left &&left, Right &&right) {
  return when_all_sender<std::remove_cvref_t<Left>, std::remove_cvref_t<Right>>{
      std::forward<Left>(left), std::forward<Right>(right)};
}

class run_loop {
  struct queue_node {
    virtual void run_node() noexcept = 0;
    virtual ~queue_node() = default;
  };

  std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<queue_node *> queue_;
  bool closed_ = false;

  enum class submit_result { ok, closed, error };

  auto submit(queue_node *node, std::exception_ptr &error) noexcept -> submit_result {
    try {
      {
        std::lock_guard lock(mutex_);
        if (closed_) {
          return submit_result::closed;
        }
        queue_.push_back(node);
      }
      cv_.notify_one();
      return submit_result::ok;
    } catch (...) {
      error = std::current_exception();
      return submit_result::error;
    }
  }

public:
  void close() noexcept {
    {
      std::lock_guard lock(mutex_);
      closed_ = true;
    }
    cv_.notify_all();
  }

  void run() noexcept {
    for (;;) {
      queue_node *node = nullptr;
      {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [&] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) {
          return;
        }
        node = queue_.front();
        queue_.pop_front();
      }
      node->run_node();
    }
  }

  struct scheduler {
    run_loop *loop_;

    struct schedule_sender {
      run_loop *loop_;

      template <class Env>
      using completion_signatures =
          mini::completion_signatures<set_value_t(), set_error_t(std::exception_ptr)>;

      template <class Receiver> struct op : queue_node {
        run_loop *loop_;
        Receiver receiver_;

        op(run_loop *loop, Receiver receiver) : loop_(loop), receiver_(std::move(receiver)) {}
        op(const op &) = delete;
        op(op &&) = delete;
        auto operator=(const op &) -> op & = delete;
        auto operator=(op &&) -> op & = delete;

        void start() noexcept {
          std::exception_ptr error;
          switch (loop_->submit(this, error)) {
          case submit_result::ok:
            return;
          case submit_result::closed:
            mini::set_error(std::move(receiver_),
                            std::make_exception_ptr(std::runtime_error("closed")));
            return;
          case submit_result::error:
            mini::set_error(std::move(receiver_), std::move(error));
            return;
          }
        }

        void run_node() noexcept override { mini::set_value(std::move(receiver_)); }
      };

      template <class Receiver> auto connect(Receiver receiver) && {
        return op<std::remove_cvref_t<Receiver>>{loop_, std::move(receiver)};
      }
    };

    auto schedule() const noexcept { return schedule_sender{loop_}; }
  };

  auto get_scheduler() noexcept { return scheduler{this}; }

  auto schedule() noexcept { return get_scheduler().schedule(); }
};

inline auto as_stdexec(just_sender<> sender) {
  (void)sender;
  return stdexec::just();
}

template <class T> inline auto as_stdexec(just_sender<T> sender) {
  return std::apply([](auto value) { return stdexec::just(std::move(value)); },
                    std::move(sender.values_));
}

} // namespace c10_p2::mini
