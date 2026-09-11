#pragma once
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include <cmath>
#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace c10_b5 {
namespace ex = stdexec;
struct ParsedRecord {
  int value{};
  double raw{};
  std::thread::id where{};
};
struct PhaseResult {
  double normalized{};
  double score{};
  std::thread::id parse_thread{};
  std::thread::id compute_thread{};
};
struct OnRoundtrip {
  int value{};
  std::thread::id outer_before_thread{};
  std::thread::id inner_thread{};
  std::thread::id after_thread{};
};
struct SwitchResult {
  std::thread::id caller_thread{};
  bool distinct_pools_observed{};
  PhaseResult starts_on;
  PhaseResult continues_on;
  OnRoundtrip on_roundtrip;
};

inline ParsedRecord parse_any(std::string text) {
  auto first = text.find(',');
  auto second = text.find(',', first == std::string::npos ? first : first + 1);
  if (first == std::string::npos || second == std::string::npos ||
      text.find(',', second + 1) != std::string::npos) {
    throw std::invalid_argument("bad record");
  }
  int value = 0;
  auto value_part = std::string_view{text}.substr(first + 1, second - first - 1);
  auto [ptr, ec] = std::from_chars(value_part.data(), value_part.data() + value_part.size(), value);
  if (ec != std::errc{} || ptr != value_part.data() + value_part.size())
    throw std::invalid_argument("bad int");
  double raw = std::stod(text.substr(second + 1));
  return ParsedRecord{value, raw, std::this_thread::get_id()};
}
inline PhaseResult finish(ParsedRecord r) {
  double n = r.raw / 100.0;
  return PhaseResult{n, r.value * n, r.where, std::this_thread::get_id()};
}
inline SwitchResult run_context_switch(std::string raw) {
  SwitchResult out{};
  out.caller_thread = std::this_thread::get_id();
  exec::static_thread_pool parse_pool(1), compute_pool(1), outer_pool(1);
  auto ps = parse_pool.get_scheduler();
  auto cs = compute_pool.get_scheduler();
  auto os = outer_pool.get_scheduler();
  auto s1 =
      ex::starts_on(ps, ex::just(raw) | ex::then(parse_any)) | ex::let_value([&](ParsedRecord r) {
        return ex::starts_on(cs, ex::just(r) | ex::then(finish));
      });
  out.starts_on = std::get<0>(*ex::sync_wait(std::move(s1)));
  auto s2 = ex::starts_on(ps, ex::just(std::move(raw)) | ex::then(parse_any) |
                                  ex::continues_on(cs) | ex::then(finish));
  out.continues_on = std::get<0>(*ex::sync_wait(std::move(s2)));
  out.distinct_pools_observed = out.starts_on.parse_thread != out.starts_on.compute_thread;
  int base = static_cast<int>(std::llround(out.starts_on.score / out.starts_on.normalized));
  auto s3 = ex::starts_on(os, ex::just(OnRoundtrip{}) | ex::then([](OnRoundtrip r) {
                                r.outer_before_thread = std::this_thread::get_id();
                                return r;
                              }) | ex::on(cs, ex::then([base](OnRoundtrip r) {
                                            r.inner_thread = std::this_thread::get_id();
                                            r.value = base;
                                            return r;
                                          })) |
                                  ex::then([](OnRoundtrip r) {
                                    r.after_thread = std::this_thread::get_id();
                                    r.value += 1;
                                    return r;
                                  }));
  out.on_roundtrip = std::get<0>(*ex::sync_wait(std::move(s3)));
  return out;
}
} // namespace c10_b5
