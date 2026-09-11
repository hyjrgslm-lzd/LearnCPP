#pragma once
#include <c10/test.hpp>
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
  throw c10::unfinished("implement controlled cancellation story");
}
} // namespace c10_c1_8
