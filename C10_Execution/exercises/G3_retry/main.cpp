#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>

#include <condition_variable>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

namespace ex = stdexec;

namespace {

template <class Operation> struct raw_slot {
  alignas(Operation) unsigned char storage[sizeof(Operation)];
  bool engaged = false;

  raw_slot() = default;
  raw_slot(const raw_slot &) = delete;
  raw_slot(raw_slot &&) = delete;
  ~raw_slot() { reset(); }

  template <class Make> auto emplace_from(Make make) -> Operation & {
    reset();
    auto *ptr = ::new (static_cast<void *>(storage)) Operation(make());
    engaged = true;
    return *ptr;
  }

  void reset() noexcept {
    if (engaged) {
      ptr()->~Operation();
      engaged = false;
    }
  }

  auto ptr() noexcept -> Operation * { return reinterpret_cast<Operation *>(storage); }
};

struct shared_script {
  int failures_before_success = 0;
  int attempts = 0;
  int live_ops = 0;
  int max_depth = 0;
  int current_depth = 0;
  bool stopped = false;
};

struct probe_state {
  ex::inplace_stop_source stop;
  int value = 0;
  int errors = 0;
  int stopped = 0;
  bool terminal = false;
  std::exception_ptr last_error;
};

struct probe_receiver {
  using receiver_concept = ex::receiver_tag;
  std::shared_ptr<probe_state> state;

  void set_value(int value) && noexcept {
    state->terminal = true;
    state->value = value;
  }

  void set_error(std::exception_ptr error) && noexcept {
    state->terminal = true;
    ++state->errors;
    state->last_error = std::move(error);
  }

  void set_stopped() && noexcept {
    state->terminal = true;
    ++state->stopped;
  }

  auto get_env() const noexcept {
    return ex::env{ex::prop{ex::get_stop_token, state->stop.get_token()}};
  }
};

struct no_assign_receiver {
  using receiver_concept = ex::receiver_tag;
  std::shared_ptr<probe_state> state;

  explicit no_assign_receiver(std::shared_ptr<probe_state> shared_state)
      : state(std::move(shared_state)) {}
  no_assign_receiver(const no_assign_receiver &) = delete;
  no_assign_receiver &operator=(const no_assign_receiver &) = delete;
  no_assign_receiver(no_assign_receiver &&) noexcept = default;
  no_assign_receiver &operator=(no_assign_receiver &&) = delete;

  void set_value(int value) && noexcept {
    state->terminal = true;
    state->value = value;
  }

  void set_error(std::exception_ptr error) && noexcept {
    state->terminal = true;
    ++state->errors;
    state->last_error = std::move(error);
  }

  void set_stopped() && noexcept {
    state->terminal = true;
    ++state->stopped;
  }

  auto get_env() const noexcept {
    return ex::env{ex::prop{ex::get_stop_token, state->stop.get_token()}};
  }
};

static_assert(std::is_move_constructible_v<no_assign_receiver>);
static_assert(!std::is_move_assignable_v<no_assign_receiver>);

struct scripted_sender {
  using sender_concept = ex::sender_tag;
  std::shared_ptr<shared_script> script_;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    std::shared_ptr<shared_script> script_;
    Receiver receiver_;
    op(std::shared_ptr<shared_script> script, Receiver receiver)
        : script_(std::move(script)), receiver_(std::move(receiver)) {
      ++script_->live_ops;
    }
    op(const op &) = delete;
    op(op &&) = delete;
    ~op() { --script_->live_ops; }
    void start() & noexcept {
      auto script = script_;
      ++script->attempts;
      ++script->current_depth;
      if (script->current_depth > script->max_depth)
        script->max_depth = script->current_depth;
      if (script->stopped) {
        ex::set_stopped(std::move(receiver_));
      } else if (script->attempts <= script->failures_before_success) {
        ex::set_error(std::move(receiver_),
                      std::make_exception_ptr(std::runtime_error("try again")));
      } else {
        ex::set_value(std::move(receiver_), script->attempts);
      }
      --script->current_depth;
    }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{script_, std::move(receiver)};
  }
};

struct script_factory {
  std::shared_ptr<shared_script> script_;
  auto operator()() const { return scripted_sender{script_}; }
};

struct manual_state {
  std::mutex mutex;
  int starts = 0;
  int live_ops = 0;
  int stop_possible_seen = 0;
  std::function<void()> complete_value;
  std::function<void()> complete_error;
  std::function<void()> complete_stopped;

  auto take(std::function<void()> &slot) -> std::function<void()> {
    std::lock_guard lock(mutex);
    auto fn = std::move(slot);
    slot = {};
    return fn;
  }

  void value() {
    auto fn = take(complete_value);
    c10::require(static_cast<bool>(fn), "manual value completion is armed");
    fn();
  }

  void error() {
    auto fn = take(complete_error);
    c10::require(static_cast<bool>(fn), "manual error completion is armed");
    fn();
  }

  void stopped_signal() {
    auto fn = take(complete_stopped);
    c10::require(static_cast<bool>(fn), "manual stopped completion is armed");
    fn();
  }
};

struct manual_sender {
  using sender_concept = ex::sender_tag;
  std::shared_ptr<manual_state> state_;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    std::shared_ptr<manual_state> state_;
    Receiver receiver_;
    int id_ = 0;

    op(std::shared_ptr<manual_state> state, Receiver receiver)
        : state_(std::move(state)), receiver_(std::move(receiver)) {}
    op(const op &) = delete;
    op(op &&) = delete;
    ~op() {
      std::lock_guard lock(state_->mutex);
      --state_->live_ops;
      if (state_->starts == id_) {
        state_->complete_value = {};
        state_->complete_error = {};
        state_->complete_stopped = {};
      }
    }

    void start() & noexcept {
      auto token = ex::get_stop_token(ex::get_env(receiver_));
      std::lock_guard lock(state_->mutex);
      id_ = ++state_->starts;
      ++state_->live_ops;
      if (token.stop_possible()) {
        ++state_->stop_possible_seen;
      }
      state_->complete_value = [this] { ex::set_value(std::move(receiver_), 42); };
      state_->complete_error = [this, token] {
        if (token.stop_requested()) {
          ex::set_stopped(std::move(receiver_));
        } else {
          ex::set_error(std::move(receiver_),
                        std::make_exception_ptr(std::runtime_error("manual error")));
        }
      };
      state_->complete_stopped = [this] { ex::set_stopped(std::move(receiver_)); };
    }
  };

  template <class Receiver> auto connect(Receiver receiver) && {
    return op<std::remove_cvref_t<Receiver>>{state_, std::move(receiver)};
  }
};

struct manual_factory {
  std::shared_ptr<manual_state> state_;
  auto operator()() const { return manual_sender{state_}; }
};

struct throwing_factory {
  std::shared_ptr<int> calls_;
  auto operator()() const -> scripted_sender {
    ++*calls_;
    throw std::runtime_error("factory failed");
  }
};

struct connect_throw_sender {
  using sender_concept = ex::sender_tag;
  std::shared_ptr<int> calls_;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver_;
    explicit op(Receiver receiver) : receiver_(std::move(receiver)) {}
    void start() & noexcept { ex::set_value(std::move(receiver_), 99); }
  };

  template <class Receiver>
  auto connect(Receiver receiver) && -> op<std::remove_cvref_t<Receiver>> {
    if (*calls_ >= 0) {
      throw std::runtime_error("connect failed");
    }
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct connect_throw_factory {
  std::shared_ptr<int> calls_;
  auto operator()() const {
    ++*calls_;
    return connect_throw_sender{calls_};
  }
};

void check_receiver_does_not_need_move_assignment() {
  {
    auto script = std::make_shared<shared_script>();
    auto result = std::make_shared<probe_state>();
    auto op = ex::connect(c10_g3::retry(script_factory{script}, 2), no_assign_receiver{result});
    ex::start(op);
    c10::require(result->terminal && result->value == 1,
                 "receiver without move assignment receives value");
  }

  {
    auto script = std::make_shared<shared_script>();
    script->failures_before_success = 3;
    auto result = std::make_shared<probe_state>();
    auto op = ex::connect(c10_g3::retry(script_factory{script}, 2), no_assign_receiver{result});
    ex::start(op);
    c10::require(result->errors == 1, "receiver without move assignment receives error");
  }

  {
    auto script = std::make_shared<shared_script>();
    script->stopped = true;
    auto result = std::make_shared<probe_state>();
    auto op = ex::connect(c10_g3::retry(script_factory{script}, 2), no_assign_receiver{result});
    ex::start(op);
    c10::require(result->stopped == 1, "receiver without move assignment receives stopped");
  }
}

void check_success_after_retries() {
  auto script = std::make_shared<shared_script>();
  script->failures_before_success = 2;
  auto result = ex::sync_wait(c10_g3::retry(script_factory{script}, 5));
  c10::require(result && std::get<0>(*result) == 3, "retry succeeds on third attempt");
  c10::require(script->attempts == 3, "retry stops after success");
  c10::require(script->live_ops == 0, "attempt operation states are released");
}

void check_exhausted_error() {
  auto script = std::make_shared<shared_script>();
  script->failures_before_success = 9;
  bool saw_error = false;
  try {
    (void)ex::sync_wait(c10_g3::retry(script_factory{script}, 3));
  } catch (const std::runtime_error &err) {
    saw_error = std::string(err.what()) == "try again";
  }
  c10::require(saw_error, "exhausted retry forwards last error");
  c10::require(script->attempts == 3, "max_attempts is total attempts");
  c10::require(script->live_ops == 0, "failed attempts are released");
}

void check_stopped_and_invalid() {
  auto script = std::make_shared<shared_script>();
  script->stopped = true;
  auto stopped = ex::sync_wait(c10_g3::retry(script_factory{script}, 5));
  c10::require(!stopped.has_value(), "stopped is not retried");
  c10::require(script->attempts == 1, "stopped consumes one attempt");

  bool rejected = false;
  try {
    (void)c10_g3::retry(script_factory{script}, 0);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  c10::require(rejected, "max_attempts <= 0 is rejected");
}

void check_stack_bounded_and_independent_connects() {
  auto run_many_failures = [](int failures) {
    auto script = std::make_shared<shared_script>();
    script->failures_before_success = failures;
    auto result = ex::sync_wait(c10_g3::retry(script_factory{script}, failures + 1));
    c10::require(result && std::get<0>(*result) == failures + 1,
                 "many synchronous failures eventually succeed");
    return script->max_depth;
  };
  int depth_100 = run_many_failures(100);
  int depth_10000 = run_many_failures(10000);
  std::cout << "sync_depth_100=" << depth_100 << " sync_depth_10000=" << depth_10000 << '\n';
  c10::require(depth_10000 == depth_100,
               "sync retry depth stays bounded and independent of attempt count: depth100=" +
                   std::to_string(depth_100) + " depth10000=" + std::to_string(depth_10000));
  c10::require(depth_10000 <= 32,
               "sync retry depth fixed budget exceeded: depth=" + std::to_string(depth_10000));

  auto fresh = std::make_shared<shared_script>();
  fresh->failures_before_success = 1;
  auto sender = c10_g3::retry(script_factory{fresh}, 3);
  auto first = ex::sync_wait(std::move(sender));
  c10::require(first && std::get<0>(*first) == 2, "first connect succeeds after one retry");

  fresh->attempts = 0;
  auto second = ex::sync_wait(c10_g3::retry(script_factory{fresh}, 3));
  c10::require(second && std::get<0>(*second) == 2, "each connect owns retry state");
}

void check_factory_throw() {
  auto calls = std::make_shared<int>(0);
  bool saw_error = false;
  try {
    (void)ex::sync_wait(c10_g3::retry(throwing_factory{calls}, 2));
  } catch (const std::runtime_error &err) {
    saw_error = std::string(err.what()) == "factory failed";
  }
  c10::require(saw_error && *calls == 2, "factory exceptions are retried then forwarded");
}

void check_connect_throw() {
  auto calls = std::make_shared<int>(0);
  bool saw_error = false;
  try {
    (void)ex::sync_wait(c10_g3::retry(connect_throw_factory{calls}, 2));
  } catch (const std::runtime_error &err) {
    saw_error = std::string(err.what()) == "connect failed";
  }
  c10::require(saw_error && *calls == 2, "connect exceptions are retried then forwarded");
}

void check_async_pending_and_cross_thread() {
  auto manual = std::make_shared<manual_state>();
  {
    auto result = std::make_shared<probe_state>();
    auto sender = c10_g3::retry(manual_factory{manual}, 3);
    auto op = ex::connect(std::move(sender), probe_receiver{result});

    ex::start(op);
    c10::require(manual->starts == 1, "async first attempt starts");
    c10::require(manual->live_ops == 1, "async attempt op remains alive while pending");
    c10::require(!result->terminal, "pending retry has not completed downstream");

    manual->error();
    c10::require(manual->starts == 2, "async error starts retry attempt");
    c10::require(manual->live_ops == 1, "completed async attempt is released before retry");

    std::jthread worker([manual] { manual->error(); });
    worker.join();
    c10::require(manual->starts == 3, "cross-thread error starts retry attempt");
    c10::require(manual->live_ops == 1, "cross-thread retry owns exactly one attempt");

    manual->value();
    c10::require(result->terminal && result->value == 42, "async retry eventually forwards value");
    c10::require(manual->live_ops == 0 || manual->live_ops == 1,
                 "completed final attempt is either cleaned before terminal return or by outer op "
                 "destruction");
  }
  c10::require(manual->live_ops == 0,
               "attempt operation states are released after outer op destruction");
}

void check_stop_before_start_and_pending_cancel() {
  {
    auto manual = std::make_shared<manual_state>();
    auto result = std::make_shared<probe_state>();
    result->stop.request_stop();
    auto sender = c10_g3::retry(manual_factory{manual}, 3);
    auto op = ex::connect(std::move(sender), probe_receiver{result});
    ex::start(op);
    c10::require(result->stopped == 1, "stop before start completes stopped");
    c10::require(manual->starts == 0, "stop before start does not create an attempt");
  }

  auto manual = std::make_shared<manual_state>();
  auto result = std::make_shared<probe_state>();
  auto sender = c10_g3::retry(manual_factory{manual}, 3);
  auto op = ex::connect(std::move(sender), probe_receiver{result});
  ex::start(op);
  c10::require(manual->stop_possible_seen == 1,
               "stop token is forwarded through retry receiver env");
  result->stop.request_stop();
  manual->error();
  c10::require(result->stopped == 1, "pending stop is forwarded as stopped");
  c10::require(manual->starts == 1, "stopped completion is not retried");
}

void check_terminal_can_destroy_outer_op() {
  auto manual = std::make_shared<manual_state>();
  auto result = std::make_shared<probe_state>();
  auto sender = c10_g3::retry(manual_factory{manual}, 2);

  struct destroy_receiver {
    using receiver_concept = ex::receiver_tag;
    std::shared_ptr<probe_state> state;
    std::function<void()> destroy;

    void set_value(int value) && noexcept {
      state->terminal = true;
      state->value = value;
      destroy();
    }
    void set_error(std::exception_ptr error) && noexcept {
      state->terminal = true;
      state->last_error = std::move(error);
      destroy();
    }
    void set_stopped() && noexcept {
      state->terminal = true;
      ++state->stopped;
      destroy();
    }
    auto get_env() const noexcept {
      return ex::env{ex::prop{ex::get_stop_token, state->stop.get_token()}};
    }
  };

  using sender_t = decltype(sender);
  using receiver_t = destroy_receiver;
  using op_t = ex::connect_result_t<sender_t, receiver_t>;
  raw_slot<op_t> op;
  receiver_t receiver{result, [&] { op.reset(); }};
  auto &started =
      op.emplace_from([&] { return ex::connect(std::move(sender), std::move(receiver)); });
  ex::start(started);
  c10::require(manual->live_ops == 1, "self-destroy case has pending attempt");
  manual->value();
  c10::require(!op.engaged, "completion receiver can destroy outer op");
  c10::require(result->terminal && result->value == 42,
               "terminal completion happens before destruction returns");
}

} // namespace

int main() {
  return c10::test_main([] {
    check_receiver_does_not_need_move_assignment();
    check_success_after_retries();
    check_exhausted_error();
    check_stopped_and_invalid();
    check_stack_bounded_and_independent_connects();
    check_factory_throw();
    check_connect_throw();
    check_async_pending_and_cross_thread();
    check_stop_before_start_and_pending_cancel();
    check_terminal_can_destroy_outer_op();
  });
}
