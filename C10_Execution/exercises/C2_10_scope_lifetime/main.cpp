#include <c10/test.hpp>
#include <solution.hpp>

#include <algorithm>
#include <string>

namespace {

bool has_event(const c10_c2_10::scope_report &report, std::string_view event) {
  return std::ranges::find(report.events, event) != report.events.end();
}

void check_scope_drains_accepted_work() {
  auto report = c10_c2_10::run_scope_story();
  c10::require(report.spawned == 3, "scope accepts fire-and-forget work");
  c10::require(report.completed == 3, "scope.on_empty drains accepted work");
  c10::require(report.future_sum == 30, "spawn_future result is retrieved");
  c10::require(report.destroyed_after_empty, "parent scope releases state after drain");
  c10::require(report.error_message == "child failed", "spawn_future preserves child error");
  c10::require(has_event(report, "spawn-1") && has_event(report, "done-3"),
               "events prove accepted work ran before release");
}

} // namespace

int main() {
  return c10::test_main([] { check_scope_drains_accepted_work(); });
}
