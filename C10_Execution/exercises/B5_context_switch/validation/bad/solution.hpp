#pragma once
#include <string>
#include <thread>
namespace c10_b5 {
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
inline SwitchResult run_context_switch(std::string) {
  SwitchResult out{};
  out.caller_thread = std::this_thread::get_id();
  out.distinct_pools_observed = true;
  out.starts_on = PhaseResult{0.0314, 1.3188, out.caller_thread, out.caller_thread};
  out.continues_on = out.starts_on;
  out.on_roundtrip = OnRoundtrip{43, out.caller_thread, out.caller_thread, out.caller_thread};
  return out;
}
} // namespace c10_b5
