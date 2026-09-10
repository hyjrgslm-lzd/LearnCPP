#pragma once
#include <cstdint>
template<int N>
__declspec(noinline) std::uint32_t transform(std::uint32_t value) {
    std::uint32_t result = value + static_cast<std::uint32_t>(N);
    for (int i = 0; i != 96; ++i) {
        result = (result * 31u) ^ (static_cast<std::uint32_t>(N) + static_cast<std::uint32_t>(i) * 17u);
    }
    return result;
}
extern template std::uint32_t transform<64>(std::uint32_t);
