#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>

#include <atomic>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <variant>

namespace mini = c10_p2::mini;
namespace ex = stdexec;

namespace {
struct recording_receiver {
  int *value;
  bool *completed;

  void set_value(int v) noexcept {
    *value = v;
    *completed = true;
  }
  void set_error(std::exception_ptr error) { std::rethrow_exception(error); }
  void set_stopped() noexcept {}
};

struct move_only_receiver {
  std::shared_ptr<int> value;
  std::shared_ptr<bool> completed;
  std::shared_ptr<bool> saw_closed;
  std::unique_ptr<int> token = std::make_unique<int>(1);

  move_only_receiver(std::shared_ptr<int> v, std::shared_ptr<bool> c, std::shared_ptr<bool> e)
      : value(std::move(v)), completed(std::move(c)), saw_closed(std::move(e)) {}
  move_only_receiver(move_only_receiver &&) noexcept = default;
  move_only_receiver &operator=(move_only_receiver &&) noexcept = default;
  move_only_receiver(const move_only_receiver &) = delete;
  move_only_receiver &operator=(const move_only_receiver &) = delete;

  void set_value() noexcept {
    *value = 1;
    *completed = true;
  }
  void set_error(std::exception_ptr error) {
    try {
      std::rethrow_exception(error);
    } catch (const std::runtime_error &e) {
      *saw_closed = std::string(e.what()) == "closed";
      *completed = true;
    }
  }
  void set_stopped() noexcept {}
};

struct allocator_env {
  int id;
  int get_allocator() const noexcept { return id; }
};

struct allocator_receiver {
  int *value;
  bool *completed;

  void set_value(int v) noexcept {
    *value = v;
    *completed = true;
  }
  void set_error(std::exception_ptr error) { std::rethrow_exception(error); }
  void set_stopped() noexcept {}
  allocator_env get_env() const noexcept { return {41}; }
};

struct env_probe_sender {
  template <class Env>
  using completion_signatures = mini::completion_signatures<mini::set_value_t(int)>;

  template <class Receiver> struct op {
    Receiver receiver;

    explicit op(Receiver r) : receiver(std::move(r)) {}
    op(const op &) = delete;
    op(op &&) = delete;
    auto operator=(const op &) -> op & = delete;
    auto operator=(op &&) -> op & = delete;

    void start() noexcept {
      auto env = mini::get_env(receiver);
      mini::set_value(std::move(receiver), mini::get_allocator(env));
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

int value_pipeline(int input, std::atomic<int> &calls) {
  auto sender = mini::then(mini::then(mini::just(input),
                                      [&](int value) {
                                        ++calls;
                                        return value * 2;
                                      }),
                           [&](int value) {
                             ++calls;
                             return value + 2;
                           });
  auto result = mini::sync_wait(std::move(sender));
  return std::get<0>(*result);
}

void check_static_protocol() {
  static_assert(std::is_empty_v<decltype(mini::connect)>);
  static_assert(std::is_empty_v<decltype(mini::start)>);
  static_assert(std::is_empty_v<decltype(mini::set_value)>);
  static_assert(std::is_empty_v<decltype(mini::set_error)>);
  static_assert(std::is_empty_v<decltype(mini::set_stopped)>);
  static_assert(std::is_empty_v<decltype(mini::get_env)>);

  using just_t = decltype(mini::just(1));
  static_assert(mini::sender<just_t>);
  static_assert(std::same_as<mini::completion_signatures_of_t<just_t>,
                             mini::completion_signatures<mini::set_value_t(int)>>);
  static_assert(std::same_as<mini::value_types_of_t<just_t>, std::variant<std::tuple<int>>>);

  using err_t = decltype(mini::just_error(std::exception_ptr{}));
  static_assert(std::same_as<mini::error_types_of_t<err_t>, std::variant<std::exception_ptr>>);

  using then_t = decltype(mini::then(mini::just(1), [](int v) -> long { return v + 1L; }));
  static_assert(std::same_as<mini::completion_signatures_of_t<then_t>,
                             mini::completion_signatures<mini::set_value_t(long),
                                                         mini::set_error_t(std::exception_ptr)>>);

  using void_then_t = decltype(mini::then(mini::just(), [] {}));
  static_assert(std::same_as<mini::value_types_of_t<void_then_t>, std::variant<std::tuple<>>>);

  auto op = mini::connect(mini::just(1), recording_receiver{nullptr, nullptr});
  static_assert(mini::operation_state<decltype(op)>);
}

void check_values_and_channels() {
  std::atomic<int> calls = 0;
  c10::require(value_pipeline(20, calls) == 42, "mini then invokes chained functions");
  c10::require(value_pipeline(3, calls) == 8, "mini pipeline consumes fresh input");
  c10::require(calls.load() == 4, "mini then executes callbacks exactly once per stage");

  auto zero = mini::sync_wait(mini::then(mini::just(), [] { return 9; }));
  c10::require(zero && std::get<0>(*zero) == 9, "mini just() sends set_value()");

  bool saw_void = false;
  auto void_result = mini::sync_wait(mini::then(mini::just(7), [&](int v) { saw_void = v == 7; }));
  c10::require(void_result && saw_void, "mini then turns void result into set_value()");

  bool saw_error = false;
  try {
    (void)mini::sync_wait(mini::just_error(std::make_exception_ptr(std::runtime_error("boom"))));
  } catch (const std::runtime_error &e) {
    saw_error = std::string(e.what()) == "boom";
  }
  c10::require(saw_error, "mini just_error reaches sync_wait");

  auto stopped = mini::sync_wait(mini::just_stopped());
  c10::require(!stopped.has_value(), "mini just_stopped reaches sync_wait");
}

void check_connect_env_pipe_and_interop() {
  int direct = 0;
  bool completed = false;
  auto op = mini::connect(mini::then(mini::just(5), [](int value) { return value + 4; }),
                          recording_receiver{&direct, &completed});
  mini::start(op);
  c10::require(completed && direct == 9, "connect/start drives receiver completion");

  int env_value = 0;
  bool env_completed = false;
  auto env_op = mini::connect(mini::then(env_probe_sender{}, [](int id) { return id + 1; }),
                              allocator_receiver{&env_value, &env_completed});
  mini::start(env_op);
  c10::require(env_completed && env_value == 42, "then receiver forwards get_env and custom query");

  auto piped = mini::sync_wait(mini::just(10) | mini::then([](int v) { return v + 1; }) |
                               mini::then([](int v) { return v * 2; }));
  c10::require(piped && std::get<0>(*piped) == 22, "pipe syntax composes then in order");

  auto stdexec_value = ex::sync_wait(mini::as_stdexec(mini::just(17)));
  c10::require(stdexec_value && std::get<0>(*stdexec_value) == 17,
               "mini just adapts to stdexec sync_wait");
}

void check_run_loop_and_when_all() {
  mini::run_loop loop;
  auto sch = loop.get_scheduler();
  std::thread::id completion_thread;
  std::jthread runner([&] { loop.run(); });
  auto scheduled = mini::sync_wait(mini::then(mini::schedule(sch), [&] {
    completion_thread = std::this_thread::get_id();
    return 9;
  }));
  loop.close();
  c10::require(scheduled && std::get<0>(*scheduled) == 9,
               "mini schedule(sch) sends void then transforms");
  c10::require(completion_thread == runner.get_id(),
               "mini then after schedule completes on run_loop thread");

  mini::run_loop delayed;
  std::jthread delayed_runner([&] { delayed.run(); });
  auto joined = mini::sync_wait(mini::when_all(
      mini::then(mini::schedule(delayed.get_scheduler()), [] { return 1; }), mini::just(2)));
  delayed.close();
  c10::require(joined && std::get<0>(*joined) == 1 && std::get<1>(*joined) == 2,
               "when_all preserves sender order when right completes first");

  bool saw_when_all_error = false;
  try {
    (void)mini::sync_wait(
        mini::when_all(mini::just_error(std::make_exception_ptr(std::runtime_error("left failed"))),
                       mini::just(2)));
  } catch (const std::runtime_error &e) {
    saw_when_all_error = std::string(e.what()) == "left failed";
  }
  c10::require(saw_when_all_error, "when_all folds error channel");

  auto stopped = mini::sync_wait(mini::when_all(mini::just_stopped(), mini::just(2)));
  c10::require(!stopped.has_value(), "when_all folds stopped channel");

  mini::run_loop closed;
  closed.close();
  auto value = std::make_shared<int>(-1);
  auto completed_closed = std::make_shared<bool>(false);
  auto saw_closed = std::make_shared<bool>(false);
  auto closed_op = mini::connect(mini::schedule(closed.get_scheduler()),
                                 move_only_receiver{value, completed_closed, saw_closed});
  mini::start(closed_op);
  c10::require(*completed_closed && *saw_closed && *value == -1,
               "closed run_loop reports error through a move-only receiver");
}
} // namespace

int main() {
  return c10::test_main([] {
    check_static_protocol();
    check_values_and_channels();
    check_connect_env_pipe_and_interop();
    check_run_loop_and_when_all();
  });
}
