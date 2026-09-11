#pragma once
#include <c10/test.hpp>
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
inline std::vector<std::string> observe_before_start(int) {
  throw c10::unfinished("A3: implement observe_before_start");
}
inline UserDashboard build_dashboard(int, std::vector<std::string> &) {
  throw c10::unfinished("A3: implement build_dashboard");
}
} // namespace c10_a3
