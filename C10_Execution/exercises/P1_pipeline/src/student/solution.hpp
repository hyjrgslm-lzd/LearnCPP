#pragma once
#include <c10/test.hpp>
#include <stdexec/execution.hpp>
#include <filesystem>
#include <map>
#include <string>
#include <cstdint>

namespace c10_p1 {
namespace ex = stdexec;
struct report {
  int valid{};
  int invalid{};
  std::int64_t value_sum{};
  std::int64_t derived_sum{};
  std::map<std::string, int> by_category;
  int parse_tasks{};
  int compute_tasks{};
  int drain_tasks{};
  int env_queries{};
};
struct pipeline_stopped : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct capacity_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};
inline auto run_pipeline(const std::filesystem::path &) -> report {
  throw c10::unfinished("P1 pipeline: implement native file sender + scheduled record pipeline");
}
inline auto run_pipeline(const std::filesystem::path &, ex::inplace_stop_token) -> report {
  throw c10::unfinished("P1 pipeline: implement native file sender + scheduled record pipeline");
}
} // namespace c10_p1
