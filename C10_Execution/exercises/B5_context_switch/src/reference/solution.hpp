#pragma once
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace c10_b5 {
namespace ex = stdexec;

struct ParsedRecord {
  std::string name;
  int value{};
  double raw{};
  std::thread::id parse_thread{};
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

inline ParsedRecord parse_record(std::string text) {
  std::stringstream input(text);
  std::string name;
  std::string value_text;
  std::string raw_text;
  if (!std::getline(input, name, ',') || !std::getline(input, value_text, ',') ||
      !std::getline(input, raw_text, ',') || input.peek() != EOF) {
    throw std::invalid_argument("record must be name,value,raw");
  }
  return ParsedRecord{name, std::stoi(value_text), std::stod(raw_text), std::this_thread::get_id()};
}

inline PhaseResult compute_record(ParsedRecord rec) {
  double normalized = rec.raw / 100.0;
  return PhaseResult{normalized, rec.value * normalized, rec.parse_thread,
                     std::this_thread::get_id()};
}

inline SwitchResult run_context_switch(std::string raw) {
  SwitchResult out{};
  out.caller_thread = std::this_thread::get_id();
  exec::static_thread_pool parse_pool(1);
  exec::static_thread_pool compute_pool(1);
  exec::static_thread_pool outer_pool(1);
  auto parse_sch = parse_pool.get_scheduler();
  auto compute_sch = compute_pool.get_scheduler();
  auto outer_sch = outer_pool.get_scheduler();

  auto starts = ex::starts_on(parse_sch, ex::just(raw) | ex::then([](std::string text) {
                                           return parse_record(std::move(text));
                                         })) |
                ex::let_value([&](ParsedRecord rec) {
                  return ex::starts_on(compute_sch,
                                       ex::just(std::move(rec)) | ex::then([](ParsedRecord value) {
                                         return compute_record(std::move(value));
                                       }));
                });
  out.starts_on = std::get<0>(*ex::sync_wait(std::move(starts)));

  auto continues = ex::starts_on(
      parse_sch, ex::just(std::move(raw)) |
                     ex::then([](std::string text) { return parse_record(std::move(text)); }) |
                     ex::continues_on(compute_sch) |
                     ex::then([](ParsedRecord rec) { return compute_record(std::move(rec)); }));
  out.continues_on = std::get<0>(*ex::sync_wait(std::move(continues)));
  out.distinct_pools_observed = out.starts_on.parse_thread != out.starts_on.compute_thread;

  int roundtrip_base =
      static_cast<int>(std::llround(out.starts_on.score / out.starts_on.normalized));
  auto roundtrip =
      ex::starts_on(outer_sch, ex::just(OnRoundtrip{}) | ex::then([](OnRoundtrip rt) {
                                 rt.outer_before_thread = std::this_thread::get_id();
                                 return rt;
                               }) | ex::on(compute_sch, ex::then([roundtrip_base](OnRoundtrip rt) {
                                             rt.inner_thread = std::this_thread::get_id();
                                             rt.value = roundtrip_base;
                                             return rt;
                                           })) |
                                   ex::then([](OnRoundtrip rt) {
                                     rt.after_thread = std::this_thread::get_id();
                                     ++rt.value;
                                     return rt;
                                   }));
  out.on_roundtrip = std::get<0>(*ex::sync_wait(std::move(roundtrip)));
  return out;
}
} // namespace c10_b5
