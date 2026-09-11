#pragma once
#include <stdexec/execution.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <functional>
#include <numeric>
#include <span>
#include <vector>
namespace c10_a1 {
namespace ex = stdexec;
struct Partial {
  std::int64_t sum{};
  std::int64_t count{};
};
struct Result {
  std::int64_t stl_sum{};
  std::int64_t sender_sum{};
  std::int64_t sender_even_count{};
  std::vector<int> coverage;
};
inline Partial reduce_span(std::span<const std::int64_t> values, std::span<int> coverage,
                           std::size_t first) {
  Partial out{};
  for (std::size_t i = 0; i < values.size(); ++i) {
    ++coverage[first + i];
    auto value = values[i];
    if (value % 2 == 0) {
      out.sum += value * value;
      ++out.count;
    }
  }
  return out;
}
inline Result compare_executions(const std::vector<std::int64_t> &data) {
  Result out{};
  out.coverage.assign(data.size(), 0);
  out.stl_sum =
      std::transform_reduce(std::execution::par, data.begin(), data.end(), std::int64_t{0},
                            std::plus<>{}, [](std::int64_t v) { return v % 2 == 0 ? v * v : 0; });
  std::size_t c = data.size() / 4;
  auto branch = [&](std::size_t first, std::size_t last) {
    return ex::just(std::span<const std::int64_t>(data.data() + first, last - first),
                    std::span<int>(out.coverage), first) |
           ex::then([](auto values, auto coverage, std::size_t offset) {
             return reduce_span(values, coverage, offset);
           });
  };
  auto got = ex::sync_wait(ex::when_all(branch(0, c), branch(c, 2 * c), branch(2 * c, 3 * c),
                                        branch(3 * c, data.size())) |
                           ex::then([](Partial a, Partial b, Partial c, Partial d) {
                             return Partial{a.sum + b.sum + c.sum + d.sum,
                                            a.count + b.count + c.count + d.count};
                           }));
  if (got) {
    out.sender_sum = std::get<0>(*got).sum;
    out.sender_even_count = std::get<0>(*got).count;
  }
  return out;
}
} // namespace c10_a1
