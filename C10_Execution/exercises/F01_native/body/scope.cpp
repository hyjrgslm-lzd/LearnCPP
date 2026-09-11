#include <c10/test.hpp>
#include <probe_config.hpp>

#if C10_F01_SCOPE_OK
#include <execution>
#include <thread>
#endif

#include <iostream>

int main() {
  return c10::test_main([] {
    std::cout << "facility=scope requested_standard=" << C10_F01_REQUESTED_STANDARD
              << " flags=" << C10_F01_REQUIRED_FLAGS << '\n';
#if C10_F01_SCOPE_OK
    namespace ex = std::execution;
    ex::counting_scope scope;
    int value = 0;
    ex::spawn(ex::just(41) | ex::then([&](int input) { value = input + 1; }), scope.get_token());
    std::this_thread::sync_wait(scope.join());
    c10::require(value == 42, "scope joined spawned work");
#else
    throw c10::skip("std::execution scope facility not available");
#endif
  });
}
