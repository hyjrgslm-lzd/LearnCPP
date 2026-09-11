#include <c10/test.hpp>
#include <solution.hpp>
#include <exec/any_sender_of.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace ex = stdexec;
using any_scheduler_completions_t =
    ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr),
                              ex::set_stopped_t()>;
using any_scheduler_receiver_t = experimental::execution::any_receiver<any_scheduler_completions_t>;
using any_scheduler_sender_t = experimental::execution::any_sender<any_scheduler_receiver_t>;
using any_scheduler_t = experimental::execution::any_scheduler<any_scheduler_sender_t>;

namespace {
template <class Operation> struct local_operation_slot {
  alignas(Operation) unsigned char storage[sizeof(Operation)];
  bool engaged = false;
  local_operation_slot() = default;
  local_operation_slot(const local_operation_slot &) = delete;
  local_operation_slot(local_operation_slot &&) = delete;
  ~local_operation_slot() { reset(); }
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

struct error_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr)>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    void start() noexcept {
      ex::set_error(std::move(receiver), std::make_exception_ptr(std::runtime_error("boom")));
    }
  };
  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct stopped_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_stopped_t()>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    void start() noexcept { ex::set_stopped(std::move(receiver)); }
  };
  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct void_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    void start() & noexcept { ex::set_value(std::move(receiver)); }
  };
  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct env_probe_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr)>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    void start() & noexcept {
      auto env = ex::get_env(receiver);
      int score = 0;
      if (ex::get_stop_token(env).stop_possible())
        score += 1;
      (void)ex::get_scheduler(env);
      score += 2;
      ex::set_value(std::move(receiver), score);
    }
  };
  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct scheduler_probe_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(std::thread::id),
                                     ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>{};
  }

  template <class Scheduler> static auto make_inner(Scheduler scheduler) {
    return ex::schedule(scheduler) | ex::then([] { return std::this_thread::get_id(); });
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    using scheduler_t = decltype(ex::get_scheduler(ex::get_env(std::declval<Receiver &>())));
    using inner_sender_t = decltype(make_inner(std::declval<scheduler_t>()));
    using inner_op_t =
        decltype(ex::connect(std::declval<inner_sender_t>(), std::declval<Receiver>()));
    Receiver receiver;
    local_operation_slot<inner_op_t> inner;

    explicit op(Receiver r) : receiver(std::move(r)) {}
    op(const op &) = delete;
    op(op &&) = delete;
    void start() & noexcept {
      try {
        auto scheduler = ex::get_scheduler(ex::get_env(receiver));
        inner.emplace_from([&] { return ex::connect(make_inner(scheduler), std::move(receiver)); });
        ex::start(*inner.ptr());
      } catch (...) {
        ex::set_error(std::move(receiver), std::current_exception());
      }
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>(std::move(receiver));
  }
};

struct manual_state {
  std::mutex mutex;
  std::condition_variable cv;
  void *op = nullptr;
  void (*complete_fn)(void *, int) noexcept = nullptr;
  bool started = false;

  void wait_started() {
    std::unique_lock lock(mutex);
    cv.wait(lock, [&] { return started; });
  }

  void complete(int value) {
    void *local_op;
    void (*local_complete)(void *, int) noexcept;
    {
      std::lock_guard lock(mutex);
      local_op = op;
      local_complete = complete_fn;
    }
    local_complete(local_op, value);
  }
};

struct manual_sender {
  manual_state *state = nullptr;
  using sender_concept = ex::sender_tag;
  template <class Self, class Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr)>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    manual_state *state;
    Receiver receiver;
    static void complete_value(void *self, int value) noexcept {
      ex::set_value(std::move(static_cast<op *>(self)->receiver), value);
    }
    void start() & noexcept {
      {
        std::lock_guard lock(state->mutex);
        state->op = this;
        state->complete_fn = complete_value;
        state->started = true;
      }
      state->cv.notify_all();
    }
  };
  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{state, std::move(receiver)};
  }
};

struct stop_observing_manual_sender {
  manual_state *state = nullptr;
  using sender_concept = ex::sender_tag;
  template <class Self, class Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }
  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    manual_state *state;
    Receiver receiver;
    ex::inplace_stop_token token{};
    static void complete_value(void *self, int value) noexcept {
      auto *operation = static_cast<op *>(self);
      if (operation->token.stop_requested()) {
        ex::set_stopped(std::move(operation->receiver));
      } else {
        ex::set_value(std::move(operation->receiver), value);
      }
    }
    void start() & noexcept {
      auto env_token = ex::get_stop_token(ex::get_env(receiver));
      if constexpr (std::is_same_v<std::remove_cvref_t<decltype(env_token)>,
                                   ex::inplace_stop_token>) {
        token = env_token;
      }
      {
        std::lock_guard lock(state->mutex);
        state->op = this;
        state->complete_fn = complete_value;
        state->started = true;
      }
      state->cv.notify_all();
    }
  };
  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{state, std::move(receiver)};
  }
};

struct terminal_state {
  std::mutex mutex;
  std::condition_variable cv;
  bool value_done = false;
  bool stopped = false;
  int value = 0;
  std::thread::id thread_id{};

  bool done() const noexcept { return value_done || stopped; }

  void wait_done() {
    std::unique_lock lock(mutex);
    cv.wait(lock, [&] { return done(); });
  }
};

struct async_result_receiver {
  using receiver_concept = ex::receiver_tag;
  terminal_state *state = nullptr;
  ex::inplace_stop_token stop_token{};
  any_scheduler_t scheduler = ex::inline_scheduler{};

  template <class Value> void set_value(Value value) && noexcept {
    {
      std::lock_guard lock(state->mutex);
      state->value_done = true;
      if constexpr (std::is_same_v<std::remove_cvref_t<Value>, std::thread::id>) {
        state->thread_id = value;
      } else {
        state->value = static_cast<int>(value);
        state->thread_id = std::this_thread::get_id();
      }
    }
    state->cv.notify_all();
  }
  void set_error(std::exception_ptr) && noexcept {
    {
      std::lock_guard lock(state->mutex);
      state->value_done = true;
      state->value = -1;
      state->thread_id = std::this_thread::get_id();
    }
    state->cv.notify_all();
  }
  void set_stopped() && noexcept {
    {
      std::lock_guard lock(state->mutex);
      state->stopped = true;
      state->thread_id = std::this_thread::get_id();
    }
    state->cv.notify_all();
  }
  auto get_env() const noexcept {
    return ex::env{ex::prop{ex::get_stop_token, stop_token},
                   ex::prop{ex::get_scheduler, scheduler}};
  }
};

struct no_assign_state {
  std::mutex mutex;
  std::condition_variable cv;
  int value = 0;
  bool terminal = false;
  void store(int new_value) {
    {
      std::lock_guard lock(mutex);
      terminal = true;
      value = new_value;
    }
    cv.notify_all();
  }
  void wait_terminal() {
    std::unique_lock lock(mutex);
    cv.wait(lock, [&] { return terminal; });
  }
};

struct no_assign_receiver {
  using receiver_concept = ex::receiver_tag;
  std::shared_ptr<no_assign_state> state;
  explicit no_assign_receiver(std::shared_ptr<no_assign_state> s) : state(std::move(s)) {}
  no_assign_receiver(const no_assign_receiver &) = delete;
  no_assign_receiver &operator=(const no_assign_receiver &) = delete;
  no_assign_receiver(no_assign_receiver &&) noexcept = default;
  no_assign_receiver &operator=(no_assign_receiver &&) = delete;
  void set_value(int value) && noexcept { state->store(value); }
  void set_error(std::exception_ptr) && noexcept { state->store(-1); }
  void set_stopped() && noexcept { state->store(-2); }
  auto get_env() const noexcept { return ex::env<>{}; }
};

struct destroying_receiver {
  using receiver_concept = ex::receiver_tag;
  std::shared_ptr<no_assign_state> state;
  std::function<void()> destroy_owner;
  destroying_receiver(std::shared_ptr<no_assign_state> s, std::function<void()> destroy)
      : state(std::move(s)), destroy_owner(std::move(destroy)) {}
  destroying_receiver(const destroying_receiver &) = delete;
  destroying_receiver &operator=(const destroying_receiver &) = delete;
  destroying_receiver(destroying_receiver &&) noexcept = default;
  destroying_receiver &operator=(destroying_receiver &&) = delete;
  void set_value(int value) && noexcept {
    state->store(value);
    destroy_owner();
  }
  void set_error(std::exception_ptr) && noexcept {
    state->store(-1);
    destroy_owner();
  }
  void set_stopped() && noexcept {
    state->store(-2);
    destroy_owner();
  }
  auto get_env() const noexcept { return ex::env<>{}; }
};

static_assert(std::is_move_constructible_v<no_assign_receiver>);
static_assert(!std::is_move_assignable_v<no_assign_receiver>);

struct immovable_sender {
  int value = 0;

  using sender_concept = ex::sender_tag;
  template <class Self, class Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    int value;

    op(Receiver r, int v) : receiver(std::move(r)), value(v) {}
    op(const op &) = delete;
    op(op &&) = delete;
    void start() & noexcept { ex::set_value(std::move(receiver), value); }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>(std::move(receiver), value);
  }
};

struct async_context {
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<std::thread> workers;
  int joined = 0;
  bool terminal_sent = false;

  ~async_context() { join_all(); }

  template <class F> void launch(F &&f) {
    std::lock_guard lock(mutex);
    workers.emplace_back(std::forward<F>(f));
  }

  void mark_terminal() {
    {
      std::lock_guard lock(mutex);
      terminal_sent = true;
    }
    cv.notify_all();
  }

  void wait_terminal() {
    std::unique_lock lock(mutex);
    cv.wait(lock, [&] { return terminal_sent; });
  }

  void join_all() {
    std::vector<std::thread> local;
    {
      std::lock_guard lock(mutex);
      local.swap(workers);
    }
    for (auto &worker : local) {
      if (worker.joinable()) {
        worker.join();
        std::lock_guard lock(mutex);
        ++joined;
      }
    }
    cv.notify_all();
  }
};

struct async_value_sender {
  async_context *context = nullptr;
  int value = 0;

  using sender_concept = ex::sender_tag;
  template <class Self, class Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr)>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    async_context *context;
    int value;
    Receiver receiver;

    void start() noexcept {
      try {
        auto *owner = context;
        int result = value;
        owner->launch([owner, result, receiver = std::move(receiver)]() mutable {
          ex::set_value(std::move(receiver), result);
          owner->mark_terminal();
        });
      } catch (...) {
        ex::set_error(std::move(receiver), std::current_exception());
      }
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{context, value, std::move(receiver)};
  }
};

c10_h3::task<int> sync_task(int input) {
  int value = co_await ex::just(input);
  co_return value + 2;
}

c10_h3::task<int> async_task(async_context &context, int input) {
  int value = co_await async_value_sender{&context, input};
  co_return value + 1;
}

c10_h3::task<int> error_task() {
  co_await error_sender{};
  co_return 0;
}

c10_h3::task<int> stopped_task() {
  co_await stopped_sender{};
  co_return 0;
}

c10_h3::task<int> immovable_task(int input) {
  int value = co_await immovable_sender{input};
  co_return value + 1;
}

c10_h3::task<std::exception_ptr> exception_ptr_value_task() {
  std::exception_ptr value =
      co_await ex::just(std::make_exception_ptr(std::runtime_error("payload")));
  co_return value;
}

c10_h3::task<int> nested_task(int input) {
  int value = co_await sync_task(input);
  co_return value + 3;
}

c10_h3::task<int> nested_error_task() {
  int value = co_await error_task();
  co_return value;
}

c10_h3::task<int> nested_stopped_task() {
  int value = co_await stopped_task();
  co_return value;
}

c10_h3::task<int> void_sender_task() {
  co_await void_sender{};
  co_return 17;
}

c10_h3::task<void> void_task() {
  co_await ex::just();
  co_return;
}

c10_h3::task<int> env_task() {
  int score = co_await env_probe_sender{};
  co_return score;
}

c10_h3::task<int> manual_task(manual_state &state) {
  int value = co_await manual_sender{&state};
  co_return value + 1;
}

c10_h3::task<int> live_stop_task(manual_state &state) {
  int value = co_await stop_observing_manual_sender{&state};
  co_return value;
}

c10_h3::task<std::thread::id> scheduler_task() { co_return co_await scheduler_probe_sender{}; }
} // namespace

int main() {
  return c10::test_main([] {
    c10::require(std::move(sync_task(40)).sync_wait() == 42,
                 "task awaits synchronous sender value");
    c10::require(std::move(sync_task(5)).sync_wait() == 7, "task computes from fresh input");
    c10::require(std::move(immovable_task(10)).sync_wait() == 11,
                 "task stores an immovable operation state without moving it");
    bool saw_exception_ptr_value = false;
    try {
      std::rethrow_exception(std::move(exception_ptr_value_task()).sync_wait());
    } catch (const std::runtime_error &e) {
      saw_exception_ptr_value = std::string(e.what()) == "payload";
    }
    c10::require(saw_exception_ptr_value, "exception_ptr value stays on the value channel");

    c10::require(std::move(nested_task(37)).sync_wait() == 42,
                 "task co_awaits another task through continuation");
    c10::require(std::move(void_sender_task()).sync_wait() == 17,
                 "task awaits void sender and resumes with void await_resume");
    std::move(void_task()).sync_wait();
    c10::require(std::move(env_task()).sync_wait() == 3,
                 "bridge receiver get_env forwards stop token and scheduler from promise");

    auto task_sender_result = ex::sync_wait(sync_task(40));
    c10::require(task_sender_result && std::get<0>(*task_sender_result) == 42,
                 "task itself is consumable as a sender");

    manual_state manual;
    terminal_state manual_terminal;
    auto manual_op = ex::connect(manual_task(manual), async_result_receiver{&manual_terminal});
    ex::start(manual_op);
    manual.wait_started();
    {
      std::lock_guard lock(manual_terminal.mutex);
      c10::require(!manual_terminal.done(),
                   "task sender start returns before a pending awaited sender completes");
    }
    manual.complete(41);
    manual_terminal.wait_done();
    c10::require(manual_terminal.value_done && manual_terminal.value == 42,
                 "task sender completes after controlled asynchronous sender completion");

    ex::inplace_stop_source stop_source;
    stop_source.request_stop();
    manual_state stopped_manual;
    terminal_state stopped_terminal;
    auto stopped_op = ex::connect(
        manual_task(stopped_manual),
        async_result_receiver{&stopped_terminal, stop_source.get_token(), ex::inline_scheduler{}});
    ex::start(stopped_op);
    stopped_terminal.wait_done();
    c10::require(
        stopped_terminal.stopped && !stopped_manual.started,
        "task sender observes a pre-requested receiver stop token before starting the coroutine");

    ex::inplace_stop_source live_stop_source;
    manual_state live_stop_manual;
    terminal_state live_stop_terminal;
    auto live_stop_op =
        ex::connect(live_stop_task(live_stop_manual),
                    async_result_receiver{&live_stop_terminal, live_stop_source.get_token(),
                                          ex::inline_scheduler{}});
    ex::start(live_stop_op);
    live_stop_manual.wait_started();
    live_stop_source.request_stop();
    live_stop_manual.complete(9);
    live_stop_terminal.wait_done();
    c10::require(live_stop_terminal.stopped,
                 "awaited sender observes live receiver inplace_stop_token after task start");

    exec::static_thread_pool pool{1};
    terminal_state scheduled_terminal;
    auto scheduled_op = ex::connect(
        scheduler_task(), async_result_receiver{&scheduled_terminal, {}, pool.get_scheduler()});
    auto caller_thread = std::this_thread::get_id();
    ex::start(scheduled_op);
    scheduled_terminal.wait_done();
    c10::require(scheduled_terminal.value_done && scheduled_terminal.thread_id != caller_thread,
                 "task sender propagates receiver scheduler into awaited sender environment");

    auto no_assign = std::make_shared<no_assign_state>();
    auto no_assign_op = ex::connect(sync_task(40), no_assign_receiver{no_assign});
    ex::start(no_assign_op);
    no_assign->wait_terminal();
    c10::require(no_assign->terminal && no_assign->value == 42,
                 "task sender receiver need not be move-assignable");

    auto destroying_state = std::make_shared<no_assign_state>();
    using destroying_op_t =
        decltype(ex::connect(sync_task(40), destroying_receiver{destroying_state, [] {}}));
    alignas(destroying_op_t) unsigned char destroying_storage[sizeof(destroying_op_t)];
    destroying_op_t *destroying_op = nullptr;
    auto destroy_operation = [&] {
      destroying_op->~destroying_op_t();
      destroying_op = nullptr;
    };
    destroying_op = ::new (static_cast<void *>(destroying_storage)) destroying_op_t(
        ex::connect(sync_task(40), destroying_receiver{destroying_state, destroy_operation}));
    ex::start(*destroying_op);
    destroying_state->wait_terminal();
    c10::require(destroying_state->terminal && destroying_state->value == 42,
                 "task sender does not access operation state after terminal destroys it");

    bool saw_nested_error = false;
    try {
      (void)std::move(nested_error_task()).sync_wait();
    } catch (const std::runtime_error &e) {
      saw_nested_error = std::string(e.what()) == "boom";
    }
    c10::require(saw_nested_error, "nested task forwards error to awaiting parent");

    bool saw_nested_stopped = false;
    try {
      (void)std::move(nested_stopped_task()).sync_wait();
    } catch (const c10_h3::stopped &) {
      saw_nested_stopped = true;
    }
    c10::require(saw_nested_stopped, "nested task forwards stopped separately");

    async_context context;
    c10::require(std::move(async_task(context, 6)).sync_wait() == 7,
                 "task resumes after cross-thread completion");
    context.wait_terminal();
    context.join_all();
    c10::require(context.joined == 1 && context.terminal_sent,
                 "checker-owned worker is really joined after terminal completion");

    bool saw_error = false;
    try {
      (void)std::move(error_task()).sync_wait();
    } catch (const std::runtime_error &e) {
      saw_error = std::string(e.what()) == "boom";
    }
    c10::require(saw_error, "await_resume rethrows sender error");

    bool saw_stopped = false;
    try {
      (void)std::move(stopped_task()).sync_wait();
    } catch (const c10_h3::stopped &) {
      saw_stopped = true;
    }
    c10::require(saw_stopped, "stopped stays separate from error");
  });
}
