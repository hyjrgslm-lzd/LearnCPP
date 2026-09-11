#pragma once
#include <c10/test.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace c10_a1 {
struct Result {
  std::int64_t stl_sum{};
  std::int64_t sender_sum{};
  std::int64_t sender_even_count{};
  std::vector<int> coverage;
};
inline Result compare_executions(const std::vector<std::int64_t> &) {
  throw c10::unfinished("A1: implement compare_executions");
}
} // namespace c10_a1
