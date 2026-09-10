#include <array>
#include <cstddef>
#include <iostream>
#include <memory_resource>
#include <new>
#include <check.hpp>

int main() {
    alignas(64) std::array<std::byte, 64> storage{};
    bool reset_observed = false;
    bool allocation_failed = false;
    {
        std::pmr::monotonic_buffer_resource resource(storage.data(), storage.size(), std::pmr::null_memory_resource());
        (void)resource.allocate(64, 16);
        resource.release();
        try {
            (void)resource.allocate(64, 16);
            reset_observed = true;
        } catch (const std::bad_alloc&) {
            // Deliberate observation of a library conformance difference, not a
            // claimed PASS for the C++23/LWG3120 reset requirement.
            allocation_failed = true;
        }
    }
    for (int batch = 0; batch < 2; ++batch) {
        std::pmr::monotonic_buffer_resource resource(storage.data(), storage.size(), std::pmr::null_memory_resource());
        check(resource.allocate(64, 16) != nullptr, "fresh resource scope provides the supplied initial buffer");
    }
    std::cout << "{\"observation\":\"LWG3120 initial buffer reset\",\"standard_expected_reset\":true,"
        << "\"observed_reset\":" << (reset_observed ? "true" : "false")
        << ",\"second_allocation_bad_alloc\":" << (allocation_failed ? "true" : "false")
        << ",\"fresh_scope_control\":true}\n";
}
