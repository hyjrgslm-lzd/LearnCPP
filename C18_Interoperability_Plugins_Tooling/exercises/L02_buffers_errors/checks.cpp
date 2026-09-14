#include "contract.hpp"
#include "solution.hpp"
#include <array>
#include <iostream>
int main() {
    const std::array<unsigned char, 5> input{'a', 0, 'z', 255, 'A'};
    std::array<unsigned char, 7> output{};
    output.fill(0x5a);
    const auto before = output;
    auto result = transform(input.data(), input.size(), output.data(), 2);
    if (output != before) { std::cerr << "check failed: failure leaves output unchanged\n"; return 1; }
    if (result.status != Status::too_small || result.written != input.size()) {
        std::cerr << "check failed: capacity status and required length\n"; return 1;
    }
    result = transform(input.data(), input.size(), output.data(), output.size());
    const std::array<unsigned char, 7> expected{'A', 0, 'Z', 255, 'A', 0x5a, 0x5a};
    if (result.status != Status::ok || result.written != input.size() || output != expected) {
        std::cerr << "check failed: binary result and canary\n"; return 1;
    }
    if (transform(nullptr, 0, nullptr, 0).status != Status::ok ||
        transform(nullptr, 1, output.data(), output.size()).status != Status::invalid ||
        transform(input.data(), input.size(), nullptr, input.size()).status != Status::invalid) {
        std::cerr << "check failed: pointer and empty contract\n"; return 1;
    }
    std::array<unsigned char, 7> overlap{'a', 'b', 'c', 'd', 'e', 'f', 0};
    result = transform(overlap.data(), 5, overlap.data() + 1, 5);
    const std::array<unsigned char, 7> overlap_expected{'a', 'A', 'B', 'C', 'D', 'E', 0};
    if (result.status != Status::ok || overlap != overlap_expected) {
        std::cerr << "check failed: overlapping buffers\n"; return 1;
    }
    std::cout << "buffer ownership and transaction: PASS\n";
}
