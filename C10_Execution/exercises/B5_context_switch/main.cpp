#include <c10/test.hpp>
#include <solution.hpp>
#include <cmath>
#include <string>
#include <thread>

namespace {
void check_record(const std::string &raw, int expected_value, double expected_raw) {
  auto result = c10_b5::run_context_switch(raw);
  double expected_normalized = expected_raw / 100.0;
  double expected_score = expected_value * expected_normalized;
  c10::require(result.starts_on.normalized == result.continues_on.normalized &&
                   result.starts_on.score == result.continues_on.score,
               "starts_on and continues_on produce same values");
  c10::require(std::abs(result.starts_on.normalized - expected_normalized) < 0.00001,
               "normalized value computed from input");
  c10::require(std::abs(result.starts_on.score - expected_score) < 0.00001,
               "score computed from input");
  c10::require(result.starts_on.parse_thread != result.caller_thread,
               "starts_on parse leaves caller thread");
  c10::require(result.starts_on.compute_thread != result.caller_thread,
               "starts_on compute leaves caller thread");
  c10::require(result.continues_on.compute_thread != result.caller_thread,
               "continues_on moves following work off caller thread");
  c10::require(result.starts_on.parse_thread != result.starts_on.compute_thread,
               "parse and compute phases use distinct scheduler threads");
  c10::require(result.distinct_pools_observed ==
                   (result.starts_on.parse_thread != result.starts_on.compute_thread),
               "distinct pool flag is derived from observed thread ids");
  c10::require(result.on_roundtrip.outer_before_thread != result.caller_thread,
               "on roundtrip starts from outer scheduler");
  c10::require(result.on_roundtrip.inner_thread != result.on_roundtrip.outer_before_thread,
               "on inner work runs on target scheduler");
  c10::require(result.on_roundtrip.after_thread == result.on_roundtrip.outer_before_thread,
               "on continuation returns to outer scheduler");
  c10::require(result.on_roundtrip.value == expected_value + 1,
               "on roundtrip preserves value flow");
}
} // namespace

int main() {
  return c10::test_main([] {
    check_record("sensor_a,42,3.14", 42, 3.14);
    check_record("sensor_b,7,12.5", 7, 12.5);
    check_record("sensor_c,-4,250", -4, 250.0);
  });
}
