#include <c10/test.hpp>
#include <solution.hpp>

#include <string>

namespace {
void check_member_first_and_fallback() {
  c10_e3::receiver rx{"rx"};
  auto member_op = c10_e3::connect(c10_e3::member_sender{10}, rx);
  auto tag_op = c10_e3::connect(c10_e3::legacy_sender{20}, rx);
  c10::require(member_op.path == "member" && member_op.value == 10,
               "member dispatch has first priority");
  c10::require(tag_op.path == "tag_invoke" && tag_op.value == 20,
               "legacy tag_invoke fallback still works");
}

void check_constraints_and_semantics() {
  static_assert(c10_e3::member_connectable<c10_e3::member_sender, c10_e3::receiver>);
  static_assert(c10_e3::legacy_connectable<c10_e3::legacy_sender, c10_e3::receiver>);
  c10::require(!c10_e3::member_connectable<c10_e3::no_connect_sender, c10_e3::receiver> &&
                   !c10_e3::legacy_connectable<c10_e3::no_connect_sender, c10_e3::receiver>,
               "missing member and legacy customization is rejected");
  c10::require(c10_e3::lookup_report() ==
                   "member -> tag_invoke; raw ADL is not in the current protocol",
               "lookup report separates current member dispatch from legacy protocol");
}
} // namespace

int main() {
  return c10::test_main([] {
    check_member_first_and_fallback();
    check_constraints_and_semantics();
  });
}
