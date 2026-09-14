#pragma once
#include <algorithm>
#include <cstddef>
inline int solve(const unsigned char* input, std::size_t length, unsigned char* output) {
    if (length == 0) return 0;
    std::transform(input, input + length, output, [](unsigned char c) {
        constexpr unsigned char lower[] = "abcdefghijklmnopqrstuvwxyz";
        constexpr unsigned char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        for (std::size_t j = 0; j < 26; ++j) if (c == lower[j]) return upper[j];
        return c;
    });
    return 0;
}
