#pragma once
#include <string>
#include <vector>

namespace c10_c1_8 {
struct cancellation_report {
  bool before_start_stopped{};
  bool during_run_stopped{};
  int after_completion_value{};
  std::string error_message;
  std::vector<std::string> events;
};
inline auto run_cancellation_story() -> cancellation_report {
  return {.before_start_stopped = true,
          .during_run_stopped = false,
          .after_completion_value = 3,
          .error_message = "boom",
          .events = {"request-before-start", "request-after-completion"}};
}
} // namespace c10_c1_8
