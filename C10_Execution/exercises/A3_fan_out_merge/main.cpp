#include <c10/test.hpp>
#include <solution.hpp>
#include <string>
#include <vector>

int main() {
  return c10::test_main([] {
    auto before = c10_a3::observe_before_start(1001);
    c10::require(before.empty(), "fan-out branches do not run while graph is only being built");

    std::vector<std::string> events;
    auto dashboard = c10_a3::build_dashboard(1001, events);
    c10::require(dashboard.profile.user_id == 1001 && dashboard.profile.name == "Alice" &&
                     dashboard.profile.level == 42,
                 "profile branch value merged");
    c10::require(dashboard.quota.user_id == 1001 && dashboard.quota.remaining == 75 &&
                     dashboard.quota.total == 100,
                 "quota branch value merged");
    c10::require(dashboard.flags.user_id == 1001 && dashboard.flags.dark_mode &&
                     !dashboard.flags.beta,
                 "flags branch value merged");
    c10::require(events == std::vector<std::string>{"profile", "quota", "flags", "merge"},
                 "branches start at consumption and merge once");
  });
}
