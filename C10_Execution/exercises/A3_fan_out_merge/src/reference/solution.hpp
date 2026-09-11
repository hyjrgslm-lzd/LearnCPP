#pragma once
#include <stdexec/execution.hpp>
#include <string>
#include <utility>
#include <vector>
namespace c10_a3 {
namespace ex = stdexec;
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
inline auto fetch_profile(int id, std::vector<std::string> &events) {
  return ex::just(id) | ex::then([&](int uid) {
           events.push_back("profile");
           return UserProfile{uid, "Alice", 42};
         });
}
inline auto fetch_quota(int id, std::vector<std::string> &events) {
  return ex::just(id) | ex::then([&](int uid) {
           events.push_back("quota");
           return QuotaState{uid, 75, 100};
         });
}
inline auto fetch_flags(int id, std::vector<std::string> &events) {
  return ex::just(id) | ex::then([&](int uid) {
           events.push_back("flags");
           return FeatureFlags{uid, true, false};
         });
}
inline std::vector<std::string> observe_before_start(int id) {
  std::vector<std::string> events;
  auto sender =
      ex::when_all(fetch_profile(id, events), fetch_quota(id, events), fetch_flags(id, events));
  (void)sender;
  return events;
}
inline UserDashboard build_dashboard(int id, std::vector<std::string> &events) {
  auto sender =
      ex::when_all(fetch_profile(id, events), fetch_quota(id, events), fetch_flags(id, events)) |
      ex::then([&](UserProfile p, QuotaState q, FeatureFlags f) {
        events.push_back("merge");
        return UserDashboard{std::move(p), std::move(q), std::move(f)};
      });
  auto got = ex::sync_wait(std::move(sender));
  return std::move(std::get<0>(*got));
}
} // namespace c10_a3
