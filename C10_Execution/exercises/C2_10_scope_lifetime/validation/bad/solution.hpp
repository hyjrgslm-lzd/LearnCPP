#pragma once
#include <string>
#include <vector>

namespace c10_c2_10 {
struct scope_report {
  int spawned{};
  int completed{};
  int future_sum{};
  bool destroyed_after_empty{};
  std::string error_message;
  std::vector<std::string> events;
};
inline auto run_scope_story() -> scope_report {
  return {.spawned = 3,
          .completed = 1,
          .future_sum = 30,
          .destroyed_after_empty = false,
          .error_message = "child failed",
          .events = {"spawn-1"}};
}
} // namespace c10_c2_10
