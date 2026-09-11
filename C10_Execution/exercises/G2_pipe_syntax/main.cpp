#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace ex = stdexec;

namespace {

template <class Sender, class Closure>
concept pipeable = requires(Sender &&sender, Closure &&closure) {
  std::forward<Sender>(sender) | std::forward<Closure>(closure);
};

template <class Left, class Right>
concept composable =
    requires(Left &&left, Right &&right) { std::forward<Left>(left) | std::forward<Right>(right); };

void check_sender_pipe() {
  auto result = ex::sync_wait(ex::just(20) | c10_g2::then([](int value) { return value + 1; }));
  c10::require(result && std::get<0>(*result) == 21, "sender | adaptor transforms value");
}

void check_adaptor_composition_order() {
  auto pipeline = c10_g2::then([](int value) { return value + 1; }) |
                  c10_g2::then([](int value) { return value * 3; }) |
                  c10_g2::then([](int value) { return std::to_string(value); });
  auto result = ex::sync_wait(ex::just(4) | std::move(pipeline));
  c10::require(result && std::get<0>(*result) == "15",
               "adaptor composition preserves left-to-right order");
}

void check_value_categories() {
  auto moved = ex::sync_wait(ex::just(std::make_unique<int>(6)) |
                             c10_g2::then([](std::unique_ptr<int> value) { return *value + 2; }));
  c10::require(moved && std::get<0>(*moved) == 8, "pipe forwards move-only values");

  auto composed = c10_g2::then([](int value) { return value + 1; }) |
                  c10_g2::then([](int value) { return value + 1; });
  auto once = ex::sync_wait(ex::just(1) | std::move(composed));
  c10::require(once && std::get<0>(*once) == 3,
               "composition closure is movable and single-use friendly");
}

void check_constraints() {
  using closure_t = decltype(c10_g2::then([](int value) { return value; }));
  static_assert(pipeable<decltype(ex::just(1)), closure_t>);
  static_assert(composable<closure_t, closure_t>);
  static_assert(!pipeable<int, closure_t>);
  c10::require(true, "pipe constraints reject non-senders");
}

} // namespace

int main() {
  return c10::test_main([] {
    check_sender_pipe();
    check_adaptor_composition_order();
    check_value_categories();
    check_constraints();
  });
}
