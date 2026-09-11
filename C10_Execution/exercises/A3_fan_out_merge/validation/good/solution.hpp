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
inline std::vector<std::string> observe_before_start(int id) {
  std::vector<std::string> events;
  auto a = ex::just(id) | ex::then([&](int uid) {
             events.push_back("profile");
             return UserProfile{uid, "Alice", 42};
           });
  auto b = ex::just(id) | ex::then([&](int uid) {
             events.push_back("quota");
             return QuotaState{uid, 75, 100};
           });
  auto c = ex::just(id) | ex::then([&](int uid) {
             events.push_back("flags");
             return FeatureFlags{uid, true, false};
           });
  (void)a;
  (void)b;
  (void)c;
  return events;
}
inline UserDashboard build_dashboard(int id, std::vector<std::string> &events) {
  auto got = ex::sync_wait(ex::when_all(ex::just(id) | ex::then([&](int uid) {
                                          events.push_back("profile");
                                          return UserProfile{uid, "Alice", 42};
                                        }),
                                        ex::just(id) | ex::then([&](int uid) {
                                          events.push_back("quota");
                                          return QuotaState{uid, 75, 100};
                                        }),
                                        ex::just(id) | ex::then([&](int uid) {
                                          events.push_back("flags");
                                          return FeatureFlags{uid, true, false};
                                        })) |
                           ex::then([&](UserProfile p, QuotaState q, FeatureFlags f) {
                             events.push_back("merge");
                             return UserDashboard{std::move(p), std::move(q), std::move(f)};
                           }));
  return std::move(std::get<0>(*got));
}
} // namespace c10_a3
