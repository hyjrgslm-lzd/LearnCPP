#include <c10/test.hpp>
#include <solution.hpp>
#include <cstdint>
#include <numeric>
#include <vector>
namespace {
std::vector<std::int64_t> make_data(std::size_t n) {
  std::vector<std::int64_t> data(n);
  std::iota(data.begin(), data.end(), 1);
  return data;
}
std::int64_t oracle(const std::vector<std::int64_t> &data) {
  std::int64_t sum = 0;
  for (auto value : data)
    if (value % 2 == 0)
      sum += value * value;
  return sum;
}
void check_case(std::size_t n) {
  auto data = make_data(n);
  auto result = c10_a1::compare_executions(data);
  c10::require(result.stl_sum == oracle(data), "parallel policy result matches scalar oracle");
  c10::require(result.sender_sum == oracle(data), "sender graph result matches scalar oracle");
  c10::require(result.sender_even_count == static_cast<std::int64_t>(n / 2),
               "sender graph counts even values");
  c10::require(result.coverage.size() == n, "coverage vector preserves input size");
  for (std::size_t i = 0; i < result.coverage.size(); ++i)
    c10::require(result.coverage[i] == 1, "four-way split covers each element exactly once");
}
} // namespace
int main() {
  return c10::test_main([] {
    check_case(0);
    check_case(1);
    check_case(7);
    check_case(16);
    check_case(100003);
  });
}
