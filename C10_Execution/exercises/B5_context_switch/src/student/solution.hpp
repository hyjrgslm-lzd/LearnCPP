#pragma once
#include <c10/test.hpp>
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
  throw c10::unfinished("B5: implement run_context_switch");
}
} // namespace c10_b5
