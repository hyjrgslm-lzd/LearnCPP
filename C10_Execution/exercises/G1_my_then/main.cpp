#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ex = stdexec;

namespace {

struct error_sender {
  using sender_concept = ex::sender_tag;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr)>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver_;

    explicit op(Receiver receiver) : receiver_(std::move(receiver)) {}
    op(const op &) = delete;
    op(op &&) = delete;

    void start() & noexcept {
      ex::set_error(std::move(receiver_), std::make_exception_ptr(std::runtime_error("boom")));
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct stopped_sender {
  using sender_concept = ex::sender_tag;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_stopped_t()>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver_;

    explicit op(Receiver receiver) : receiver_(std::move(receiver)) {}
    op(const op &) = delete;
    op(op &&) = delete;

    void start() & noexcept { ex::set_stopped(std::move(receiver_)); }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct env_box {
  int value;
};

struct int_signature_env {
  int value;
};

struct string_signature_env {
  std::string value;
};

struct env_receiver {
  using receiver_concept = ex::receiver_tag;

  int *value_;
  bool *completed_;

  void set_value(int value) noexcept {
    *value_ = value;
    *completed_ = true;
  }

  void set_error(std::exception_ptr) noexcept { std::terminate(); }

  void set_stopped() noexcept { std::terminate(); }

  auto get_env() const noexcept -> env_box { return env_box{41}; }
};

struct int_signature_receiver {
  using receiver_concept = ex::receiver_tag;

  long *value_;

  void set_value(long value) noexcept { *value_ = value; }

  void set_error(std::exception_ptr) noexcept { std::terminate(); }

  void set_stopped() noexcept { std::terminate(); }

  auto get_env() const noexcept -> int_signature_env { return int_signature_env{20}; }
};

struct string_signature_receiver {
  using receiver_concept = ex::receiver_tag;

  std::size_t *value_;

  void set_value(std::size_t value) noexcept { *value_ = value; }

  void set_error(std::exception_ptr) noexcept { std::terminate(); }

  void set_stopped() noexcept { std::terminate(); }

  auto get_env() const noexcept -> string_signature_env { return string_signature_env{"abcd"}; }
};

struct env_read_sender {
  using sender_concept = ex::sender_tag;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver_;

    explicit op(Receiver receiver) : receiver_(std::move(receiver)) {}
    op(const op &) = delete;
    op(op &&) = delete;

    void start() & noexcept { ex::set_value(std::move(receiver_), ex::get_env(receiver_).value); }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct env_signature_sender {
  using sender_concept = ex::sender_tag;

  template <class Self, class Env> static consteval auto get_completion_signatures() {
    if constexpr (std::same_as<Env, string_signature_env>) {
      return ex::completion_signatures<ex::set_value_t(std::string)>{};
    } else {
      return ex::completion_signatures<ex::set_value_t(int)>{};
    }
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    Receiver receiver_;

    explicit op(Receiver receiver) : receiver_(std::move(receiver)) {}
    op(const op &) = delete;
    op(op &&) = delete;

    void start() & noexcept {
      using env_t = decltype(ex::get_env(receiver_));
      if constexpr (std::same_as<env_t, string_signature_env>) {
        ex::set_value(std::move(receiver_), ex::get_env(receiver_).value);
      } else {
        ex::set_value(std::move(receiver_), ex::get_env(receiver_).value);
      }
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{std::move(receiver)};
  }
};

struct env_signature_fn {
  long operator()(int value) const noexcept { return value + 2L; }

  std::size_t operator()(std::string value) const noexcept { return value.size(); }
};

struct never_started {
  using operation_state_concept = ex::operation_state_tag;
  void start() & noexcept {}
};

struct throw_connect_sender {
  using sender_concept = ex::sender_tag;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }

  template <class Receiver> auto connect(Receiver) const -> never_started {
    throw std::runtime_error("connect failed");
  }
};

template <class OptionalTuple> void require_string_result(const OptionalTuple &result) {
  using value_t = std::tuple_element_t<0, std::remove_cvref_t<decltype(*result)>>;
  if constexpr (std::same_as<value_t, std::string>) {
    c10::require(result && std::get<0>(*result) == "3", "multi-value input transformed");
  } else {
    c10::require(false, "multi-value input must produce std::string");
  }
}

void check_values() {
  auto zero = ex::sync_wait(c10_g1::my_then(ex::just(), [] { return 9; }));
  c10::require(zero && std::get<0>(*zero) == 9, "zero value transformed");

  auto one = ex::sync_wait(c10_g1::my_then(ex::just(42), [](int value) { return value + 1; }));
  c10::require(one && std::get<0>(*one) == 43, "single value transformed");

  auto multi = ex::sync_wait(c10_g1::my_then(
      ex::just(1, 2), [](int left, int right) { return std::to_string(left + right); }));
  require_string_result(multi);

  bool called = false;
  auto void_result =
      ex::sync_wait(c10_g1::my_then(ex::just(7), [&](int value) { called = value == 7; }));
  c10::require(void_result && called, "void result sends set_value()");

  auto moved = ex::sync_wait(c10_g1::my_then(
      ex::just(std::make_unique<int>(5)), [](std::unique_ptr<int> value) { return *value + 1; }));
  c10::require(moved && std::get<0>(*moved) == 6, "move-only value transformed");
}

void check_error_stopped_and_throw() {
  bool saw_transform_throw = false;
  try {
    (void)ex::sync_wait(c10_g1::my_then(
        ex::just(1), [](int) -> int { throw std::runtime_error("transform failed"); }));
  } catch (const std::runtime_error &err) {
    saw_transform_throw = std::string(err.what()) == "transform failed";
  }
  c10::require(saw_transform_throw, "throwing transform becomes set_error");

  bool saw_upstream_error = false;
  try {
    (void)ex::sync_wait(c10_g1::my_then(error_sender{}, [](int value) { return value; }));
  } catch (const std::runtime_error &err) {
    saw_upstream_error = std::string(err.what()) == "boom";
  }
  c10::require(saw_upstream_error, "error channel forwarded as set_error");

  auto stopped = ex::sync_wait(c10_g1::my_then(stopped_sender{}, [](int value) { return value; }));
  c10::require(!stopped.has_value(), "stopped channel forwarded");
}

void check_environment_and_boundaries() {
  int value = 0;
  bool completed = false;
  auto op =
      ex::connect(c10_g1::my_then(env_read_sender{}, [](int env_value) { return env_value + 1; }),
                  env_receiver{&value, &completed});
  static_assert(!std::move_constructible<decltype(op)>);
  c10::require(noexcept(ex::start(op)), "start is noexcept");
  ex::start(op);
  c10::require(completed && value == 42, "receiver environment forwarded");

  bool connect_threw = false;
  try {
    (void)ex::connect(c10_g1::my_then(throw_connect_sender{}, [](int value) { return value; }),
                      env_receiver{&value, &completed});
  } catch (const std::runtime_error &err) {
    connect_threw = std::string(err.what()) == "connect failed";
  }
  c10::require(connect_threw, "connect propagates upstream construction failure");
}

void check_environment_dependent_signatures() {
  using sender_t = decltype(c10_g1::my_then(env_signature_sender{}, env_signature_fn{}));
  using int_sigs_t = ex::completion_signatures_of_t<sender_t, int_signature_env>;
  using string_sigs_t = ex::completion_signatures_of_t<sender_t, string_signature_env>;
  (void)sizeof(int_sigs_t);
  (void)sizeof(string_sigs_t);
  c10::require(ex::sender_in<sender_t, int_signature_env>,
               "int env completion signatures instantiate");
  c10::require(ex::sender_in<sender_t, string_signature_env>,
               "string env completion signatures instantiate");

  long int_value = 0;
  auto int_op = ex::connect(c10_g1::my_then(env_signature_sender{}, env_signature_fn{}),
                            int_signature_receiver{&int_value});
  ex::start(int_op);
  c10::require(int_value == 22, "int environment selects int value signature");

  std::size_t string_value = 0;
  auto string_op = ex::connect(c10_g1::my_then(env_signature_sender{}, env_signature_fn{}),
                               string_signature_receiver{&string_value});
  ex::start(string_op);
  c10::require(string_value == 4, "string environment selects string value signature");
}

void check_signature_instantiation() {
  auto sender =
      c10_g1::my_then(ex::just(1, 2), [](int left, int right) -> long { return left + right; });
  using sender_t = decltype(sender);
  using signatures_t = ex::completion_signatures_of_t<sender_t, ex::env<>>;
  (void)sizeof(signatures_t);
  c10::require(ex::sender_in<sender_t, ex::env<>>, "completion signatures instantiate");
}

} // namespace

int main() {
  return c10::test_main([] {
    using check_fn = void (*)();
    check_fn checks[] = {check_values, check_error_stopped_and_throw,
                         check_environment_and_boundaries, check_environment_dependent_signatures,
                         check_signature_instantiation};
    for (check_fn check : checks) {
      check();
    }
  });
}
