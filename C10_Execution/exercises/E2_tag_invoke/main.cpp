#include <c10/test.hpp>
#include <solution.hpp>

#include <concepts>
#include <string>

namespace {
void check_connect_start_query() {
  c10_e2::sender sender{42};
  c10_e2::receiver receiver{"rx"};
  auto op = c10_e2::connect(sender, receiver);
  static_assert(std::same_as<decltype(op), c10_e2::operation_state>);
  c10::require(op.value == 42 && op.receiver_label == "rx",
               "connect dispatches through tag_invoke");
  c10::require(!op.started, "connect remains lazy");
  c10_e2::start(op);
  c10::require(op.started, "start dispatches through tag_invoke");
  c10::require(c10_e2::get_scheduler(c10_e2::env{{"pool"}}).name == "pool",
               "query dispatches through tag_invoke");
}

void check_constraints() {
  static_assert(c10_e2::tag_invocable<c10_e2::connect_t, c10_e2::sender, c10_e2::receiver>);
  static_assert(!c10_e2::tag_invocable<c10_e2::connect_t, c10_e2::receiver, c10_e2::sender>);
  c10::require(!c10_e2::tag_invocable<c10_e2::connect_t, c10_e2::receiver, c10_e2::sender>,
               "missing tag_invoke overload is rejected by the CPO constraint");
}
} // namespace

int main() {
  return c10::test_main([] {
    check_connect_start_query();
    check_constraints();
  });
}
