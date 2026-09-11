#pragma once
#include <c10/test.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
namespace c10_b6 {
struct StageRecord {
  std::string phase;
  std::string record;
  int branch{};
  std::thread::id thread{};
};
struct Report {
  int total{};
  int valid_count{};
  int invalid_count{};
  double total_score{};
  int completed_batches{};
  std::thread::id caller_thread{};
  std::thread::id merge_thread{};
  std::vector<StageRecord> stages;
};
using EnrichHook = std::function<double(std::string_view, int)>;
inline double default_score(std::string_view name, int value) {
  return value * 1.5 + static_cast<double>(name.size());
}
inline Report run_pipeline(const std::vector<std::string> &, std::size_t, EnrichHook) {
  throw c10::unfinished("B6: implement run_pipeline");
}
inline Report run_pipeline(const std::vector<std::string> &input, std::size_t max_records) {
  return run_pipeline(input, max_records, default_score);
}
} // namespace c10_b6
