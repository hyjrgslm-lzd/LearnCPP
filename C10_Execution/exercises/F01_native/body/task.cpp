#include <c10/test.hpp>
#include <probe_config.hpp>

#if C10_F01_TASK_OK
#include <execution>
#include <thread>

std::execution::task<int> make_task() { co_return 42; }
#endif

#include <iostream>

int main() {
  return c10::test_main([] {
    std::cout << "facility=task requested_standard=" << C10_F01_REQUESTED_STANDARD
              << " flags=" << C10_F01_REQUIRED_FLAGS << '\n';
#if C10_F01_TASK_OK
    auto result = std::this_thread::sync_wait(make_task());
    c10::require(result.has_value(), "task returned value");
    auto [value] = result.value();
    c10::require(value == 42, "task co_return value");
#else
    throw c10::skip("std::execution task facility not available");
#endif
  });
}
