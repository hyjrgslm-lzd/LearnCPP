#pragma once
#include <string>
#include <vector>
namespace c10_a3 {
struct UserProfile {
  int user_id{};
  std::string name;
  int level{};
};
struct QuotaState {
  int user_id{};
  int remaining{};
  int total{};
};
struct FeatureFlags {
  int user_id{};
  bool dark_mode{};
  bool beta{};
};
struct UserDashboard {
  UserProfile profile;
  QuotaState quota;
  FeatureFlags flags;
};
inline std::vector<std::string> observe_before_start(int) { return {"profile"}; }
inline UserDashboard build_dashboard(int id, std::vector<std::string> &events) {
  events = {"profile", "quota", "flags", "merge"};
  return UserDashboard{UserProfile{id, "Alice", 42}, QuotaState{id, 75, 100},
                       FeatureFlags{id, true, false}};
}
} // namespace c10_a3
