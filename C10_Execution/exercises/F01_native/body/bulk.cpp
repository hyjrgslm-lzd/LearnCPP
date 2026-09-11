#include <c10/test.hpp>
#include <probe_config.hpp>

#if C10_F01_BULK_OK
#include <execution>
#include <thread>
#include <utility>
#endif

#include <iostream>

int main() {
  return c10::test_main([] {
    std::cout << "facility=bulk requested_standard=" << C10_F01_REQUESTED_STANDARD
              << " flags=" << C10_F01_REQUIRED_FLAGS << '\n';
#if C10_F01_BULK_OK
    namespace ex = std::execution;
    auto sender = ex::just(0) | ex::bulk(ex::par, 4, [](int, int &) {});
    auto result = std::this_thread::sync_wait(std::move(sender));
    c10::require(result.has_value(), "bulk sender completed");
#else
    throw c10::skip("std::execution bulk facility not available");
#endif
  });
}
