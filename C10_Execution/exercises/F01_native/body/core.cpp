#include <c10/test.hpp>
#include <probe_config.hpp>

#if C10_F01_CORE_OK
#include <execution>
#include <thread>
#include <utility>
#endif

#include <iostream>

int main() {
  return c10::test_main([] {
    std::cout << "facility=core requested_standard=" << C10_F01_REQUESTED_STANDARD
              << " flags=" << C10_F01_REQUIRED_FLAGS << '\n';
#if C10_F01_CORE_OK
    namespace ex = std::execution;
    auto sender =
        ex::when_all(ex::just(20), ex::just(22)) | ex::then([](int a, int b) { return a + b; });
    auto result = std::this_thread::sync_wait(std::move(sender));
    c10::require(result.has_value(), "core sender returned value");
    auto [value] = result.value();
    c10::require(value == 42, "core when_all/then value");
#else
    throw c10::skip("std::execution core sender facilities not available");
#endif
  });
}
