#include <c10/test.hpp>
#include <solution.hpp>

#include <stdexcept>
#include <string>

namespace {

void check_upon_error_recovers_to_value() {
  auto ok = c10_c1_7::parse_with_upon_error("alice,42,3.5");
  c10::require(ok.ok && ok.name == "alice" && ok.error_code == 0,
               "valid input remains on value channel");

  auto bad = c10_c1_7::parse_with_upon_error("");
  c10::require(!bad.ok && bad.name == "BadRequest" && bad.error_code == -1,
               "upon_error converts exception_ptr to fallback value");
}

void check_complete_input_consumption() {
  for (int i = 0; i < 17; ++i) {
    const auto name = "User" + std::to_string(i);
    const auto line = name + "," + std::to_string(i - 4) + ",0.125";
    c10::require(c10_c1_7::parse_with_upon_error(line).name == name,
                 "valid names are not fixture constants");
    c10::require(c10_c1_7::parse_with_let_error(line).name == name,
                 "let_error preserves valid input");
  }
  for (const char *text : {",1,2.0", "a,1x,2.0", "a,1,2.0tail", "a,+1,2.0", "a,1,nan", "a,1,inf",
                           "a,2147483648,1", "a,1,2,3"}) {
    c10::require(!c10_c1_7::parse_with_upon_error(text).ok,
                 "malformed numeric data goes through error recovery");
  }
}

void check_let_error_returns_sender() {
  auto bad = c10_c1_7::parse_with_let_error("missing-fields");
  c10::require(!bad.ok && bad.name == "RecoveredBySender" && bad.error_code == -2,
               "let_error returns fallback sender");
}

void check_later_error_is_not_swallowed() {
  bool saw_error = false;
  try {
    (void)c10_c1_7::parse_then_throw_after_recovery("");
  } catch (const std::runtime_error &err) {
    saw_error = std::string{err.what()} == "after recovery";
  }
  c10::require(saw_error, "later value-stage errors still use error channel");
}

} // namespace

int main() {
  return c10::test_main([] {
    check_upon_error_recovers_to_value();
    check_complete_input_consumption();
    check_let_error_returns_sender();
    check_later_error_is_not_swallowed();
  });
}
