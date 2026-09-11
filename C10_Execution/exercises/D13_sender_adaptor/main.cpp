#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace ex = stdexec;

namespace {

struct env_box {
  int marker;
};

struct value_sender {
  using sender_concept = ex::sender_tag;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int, int)>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    explicit op(Receiver r) : receiver(std::move(r)) {}
    op(const op &) = delete;
    op(op &&) = delete;
    void start() & noexcept { ex::set_value(std::move(receiver), 2, 5); }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct error_sender {
  using sender_concept = ex::sender_tag;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_error_t(std::exception_ptr)>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    explicit op(Receiver r) : receiver(std::move(r)) {}
    op(const op &) = delete;
    op(op &&) = delete;
    void start() & noexcept {
      ex::set_error(std::move(receiver), std::make_exception_ptr(std::runtime_error("boom")));
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct stopped_sender {
  using sender_concept = ex::sender_tag;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_stopped_t()>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    explicit op(Receiver r) : receiver(std::move(r)) {}
    op(const op &) = delete;
    op(op &&) = delete;
    void start() & noexcept { ex::set_stopped(std::move(receiver)); }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct value_probe {
  using receiver_concept = ex::receiver_tag;
  int *first{};
  int *second{};
  bool *completed{};
  void set_value(int a, int b) && noexcept {
    *first = a;
    *second = b;
    *completed = true;
  }
  void set_error(std::exception_ptr) && noexcept { std::terminate(); }
  void set_stopped() && noexcept { std::terminate(); }
  auto get_env() const noexcept { return ex::env<>{}; }
};

struct error_probe {
  using receiver_concept = ex::receiver_tag;
  std::string *message{};
  void set_value(int, int) && noexcept {}
  void set_error(std::exception_ptr error) && noexcept {
    try {
      if (error)
        std::rethrow_exception(error);
    } catch (const std::exception &err) {
      *message = err.what();
    }
  }
  void set_stopped() && noexcept {}
  auto get_env() const noexcept { return ex::env<>{}; }
};

struct env_receiver {
  using receiver_concept = ex::receiver_tag;
  int *value{};
  bool *completed{};

  void set_value(int v) && noexcept {
    *value = v;
    *completed = true;
  }
  void set_error(std::exception_ptr) && noexcept { std::terminate(); }
  void set_stopped() && noexcept { std::terminate(); }
  auto get_env() const noexcept -> env_box { return {40}; }
};

struct env_read_sender {
  using sender_concept = ex::sender_tag;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver;
    op(Receiver r) : receiver(std::move(r)) {}
    op(const op &) = delete;
    op(op &&) = delete;
    void start() & noexcept {
      ex::set_value(std::move(receiver), ex::get_env(receiver).marker + 2);
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

void check_tap_forwards_values() {
  std::vector<std::string> seen;
  int first = 0;
  int second = 0;
  bool completed = false;
  auto op = ex::connect(c10_d13::tap(value_sender{},
                                     [&](int a, int b) {
                                       seen.push_back(std::to_string(a) + "," + std::to_string(b));
                                     }),
                        value_probe{&first, &second, &completed});
  ex::start(op);
  c10::require(completed && first == 2 && second == 5, "tap forwards value arguments unchanged");
  c10::require((seen == std::vector<std::string>{"2,5"}), "tap observes value exactly once");
}

void check_error_stopped_and_throw() {
  std::string error;
  auto error_op = ex::connect(c10_d13::tap(error_sender{}, [] {}), error_probe{&error});
  ex::start(error_op);
  c10::require(error == "boom", "tap forwards error channel");

  struct stopped_probe {
    bool *stopped{};
    void set_value(int) && noexcept {}
    void set_error(std::exception_ptr) && noexcept {}
    void set_stopped() && noexcept { *stopped = true; }
    auto get_env() const noexcept { return ex::env<>{}; }
  };
  bool stopped = false;
  auto stopped_op = ex::connect(c10_d13::tap(stopped_sender{}, [] {}), stopped_probe{&stopped});
  ex::start(stopped_op);
  c10::require(stopped, "tap forwards stopped channel");

  std::string tap_error;
  auto throw_op = ex::connect(
      c10_d13::tap(value_sender{}, [](int, int) { throw std::runtime_error("tap failed"); }),
      error_probe{&tap_error});
  ex::start(throw_op);
  c10::require(tap_error == "tap failed", "tap exception becomes set_error");
}

void check_environment_and_boundaries() {
  int value = 0;
  bool completed = false;
  auto op =
      ex::connect(c10_d13::tap(env_read_sender{}, [](int) {}), env_receiver{&value, &completed});
  static_assert(!std::move_constructible<decltype(op)>);
  c10::require(noexcept(ex::start(op)), "start is noexcept");
  ex::start(op);
  c10::require(completed && value == 42, "tap receiver forwards environment");
}

} // namespace

int main() {
  return c10::test_main([] {
    check_tap_forwards_values();
    check_error_stopped_and_throw();
    check_environment_and_boundaries();
  });
}
