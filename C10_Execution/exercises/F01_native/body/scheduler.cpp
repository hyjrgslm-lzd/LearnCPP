#include <c10/test.hpp>
#include <probe_config.hpp>

#if C10_F01_SCHEDULER_OK
#include <execution>
#include <thread>
#endif

#include <iostream>

int main() {
  return c10::test_main([] {
    std::cout << "facility=scheduler requested_standard=" << C10_F01_REQUESTED_STANDARD
              << " flags=" << C10_F01_REQUIRED_FLAGS << '\n';
#if C10_F01_SCHEDULER_OK
    namespace ex = std::execution;
    ex::scheduler auto scheduler = ex::inline_scheduler{};
    auto result = std::this_thread::sync_wait(ex::schedule(scheduler));
    c10::require(result.has_value(), "scheduler sender completed");
#else
    throw c10::skip("std::execution scheduler facilities not available");
#endif
  });
}
