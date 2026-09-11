#pragma once
#include <c10/test.hpp>
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
  throw c10::unfinished("implement async_scope lifetime story");
}
} // namespace c10_c2_10
