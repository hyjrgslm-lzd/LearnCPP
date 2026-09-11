#include <c10/test.hpp>
#include <solution.hpp>

#include <algorithm>
#include <string>

namespace {

bool has_event(const c10_c1_8::cancellation_report &report, std::string_view event) {
  return std::ranges::find(report.events, event) != report.events.end();
}

void check_controlled_cancellation() {
  auto report = c10_c1_8::run_cancellation_story();
  c10::require(report.before_start_stopped, "cancel before start completes stopped");
  c10::require(report.during_run_stopped, "cancel during work completes stopped");
  c10::require(report.after_completion_value == 3, "stop after completion does not rewrite value");
  c10::require(report.error_message == "boom", "error channel remains distinct from stopped");
  c10::require(has_event(report, "request-before-start"), "before-start request recorded");
  c10::require(has_event(report, "request-during-step-2"), "during-work request recorded");
  c10::require(has_event(report, "request-after-completion"), "after-completion request recorded");
}

} // namespace

int main() {
  return c10::test_main([] { check_controlled_cancellation(); });
}
