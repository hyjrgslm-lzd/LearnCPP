#pragma once
#include <stdexec/execution.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <functional>
#include <numeric>
#include <vector>
namespace c10_a1 {
namespace ex = stdexec;
struct Result {
  std::int64_t stl_sum{};
  std::int64_t sender_sum{};
  std::int64_t sender_even_count{};
  std::vector<int> coverage;
};
struct Partial {
  std::int64_t sum{};
  std::int64_t count{};
};
inline Result compare_executions(const std::vector<std::int64_t> &data) {
  Result r{};
  r.coverage.assign(data.size(), 0);
  for (auto value : data)
    if (value % 2 == 0)
      r.stl_sum += value * value;
  std::array<std::size_t, 5> cut{0, data.size() / 4, data.size() / 2, data.size() - data.size() / 4,
                                 data.size()};
  auto part = [&](std::size_t first, std::size_t last) {
    return ex::just(first, last) | ex::then([&](std::size_t begin, std::size_t end) {
             Partial p{};
             for (auto i = begin; i < end; ++i) {
               ++r.coverage[i];
               if ((data[i] & 1) == 0) {
                 p.sum += data[i] * data[i];
                 ++p.count;
               }
             }
             return p;
           });
  };
  auto got = ex::sync_wait(ex::when_all(part(cut[0], cut[1]), part(cut[1], cut[2]),
                                        part(cut[2], cut[3]), part(cut[3], cut[4])) |
                           ex::then([](Partial a, Partial b, Partial c, Partial d) {
                             return Partial{a.sum + b.sum + c.sum + d.sum,
                                            a.count + b.count + c.count + d.count};
                           }));
  if (got) {
    r.sender_sum = std::get<0>(*got).sum;
    r.sender_even_count = std::get<0>(*got).count;
  }
  return r;
}
} // namespace c10_a1
