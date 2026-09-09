#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>

int main() {
#ifndef __cpp_lib_start_lifetime_as
    std::cout << "SKIP: __cpp_lib_start_lifetime_as not defined\n";
    return 0;
#else
    static_assert(__cpp_lib_start_lifetime_as >= 202207L);

    constexpr std::uint32_t expected = 0x12345678u;
    auto bytes = std::bit_cast<std::array<std::byte, sizeof(expected)>>(expected);
    auto* value = std::start_lifetime_as<std::uint32_t>(bytes.data());
    if (*value != expected) {
        std::cerr << "start_lifetime_as value mismatch\n";
        return 1;
    }

    alignas(std::uint16_t) std::array<std::byte, sizeof(std::uint16_t) * 3> array_bytes{};
    auto* values = std::start_lifetime_as_array<std::uint16_t>(array_bytes.data(), 3);
    values[0] = 11;
    values[1] = 22;
    values[2] = 33;
    if (values[0] + values[1] + values[2] != 66) {
        std::cerr << "start_lifetime_as_array value mismatch\n";
        return 1;
    }

    std::cout << "L12_start_lifetime_as OK macro=" << __cpp_lib_start_lifetime_as << "\n";
    return 0;
#endif
}
